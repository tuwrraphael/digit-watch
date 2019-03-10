#include "sdk_common.h"
#include "ble_bas.h"
#include <string.h>
#include "ble_srv_common.h"
#include "ble_conn_state.h"
#include "nrf_log.h"
#include "./ble_digit.h"
#include "time.h"
#include "app_util.h"
#include <stdint.h>
#include "mem_manager.h"

#define YEAR_LENGTH (2)
#define MONTH_LENGTH (1)
#define DAY_LENGTH (1)
#define HOUR_LENGTH (1)
#define MINUTE_LENGTH (1)
#define SECOND_LENGTH (1)
#define CTS_CHAR_LENGTH (YEAR_LENGTH +   \
						 MONTH_LENGTH +  \
						 DAY_LENGTH +    \
						 HOUR_LENGTH +   \
						 MINUTE_LENGTH + \
						 SECOND_LENGTH)

#define LEGS_CHAR_LENGTH (CTS_CHAR_LENGTH + 2 * STOP_LENGTH + 1 + LINE_LENGTH + DIRECTION_LENGTH + 4)

static uint8_t min(uint8_t m1, uint8_t m2)
{
	return m1 < m2 ? m1 : m2;
}

static void read_cts(const uint8_t src[CTS_CHAR_LENGTH], time_t *dest)
{
	struct tm parsed = {
		.tm_sec = src[6],
		.tm_min = src[5],
		.tm_hour = src[4],
		.tm_mday = src[3],
		.tm_mon = src[2],
		.tm_year = uint16_decode(&src[0]),
		.tm_isdst = 0};
	*dest = mktime(&parsed);
}

static void on_write(ble_digit_t *p_digit, ble_evt_t const *p_ble_evt)
{
	ble_gatts_evt_write_t const *p_evt_write = &p_ble_evt->evt.gatts_evt.params.write;
	if ((p_evt_write->handle == p_digit->cts_char_handles.value_handle))
	{
		if (p_evt_write->len == CTS_CHAR_LENGTH)
		{
			read_cts(p_evt_write->data, &p_digit->state->current_time);
			NRF_LOG_INFO("received %d = %d, %d, %d, %d, %d", p_digit->state->current_time,
						 uint16_decode(&p_evt_write->data[0]),
						 p_evt_write->data[2],
						 p_evt_write->data[3],
						 p_evt_write->data[4],
						 p_evt_write->data[5])
		}
		else
		{
			NRF_LOG_ERROR("Insufficient data length for cts: %d.", p_evt_write->len);
		}
	}
	if ((p_evt_write->handle == p_digit->event_char_handles.value_handle))
	{
		if (p_evt_write->len >= (CTS_CHAR_LENGTH + 1) || p_evt_write->len <= CTS_CHAR_LENGTH + SUBJECT_LENGTH)
		{
			read_cts(p_evt_write->data, &p_digit->state->event_start_time);
			const char *subjectPtr = (char *)p_evt_write->data + CTS_CHAR_LENGTH;
			p_digit->state->event_subject[0] = '\0';
			strncat(p_digit->state->event_subject, subjectPtr, min(SUBJECT_LENGTH - 1, p_evt_write->len - CTS_CHAR_LENGTH));
			NRF_LOG_INFO("event %s at %d", p_digit->state->event_subject, p_digit->state->event_start_time);
		}
		else
		{
			NRF_LOG_ERROR("Insufficient data length for event: %d.", p_evt_write->len);
		}
	}
	if ((p_evt_write->handle == p_digit->directions_char_handles.value_handle))
	{
		if (p_evt_write->len == (CTS_CHAR_LENGTH * 2 + 1))
		{
			read_cts(p_evt_write->data + 1, &p_digit->state->directions.departure_time);
			read_cts(p_evt_write->data + 1 + CTS_CHAR_LENGTH, &p_digit->state->directions.arrival_time);
			p_digit->state->directions.legs_count = p_evt_write->data[0];
			p_digit->state->directions.valid_legs = 0;
			if (NULL != p_digit->state->directions.legs)
			{
				nrf_free(p_digit->state->directions.legs);
			}
			if (p_digit->state->directions.legs_count > MAX_LEGS_COUNT)
			{
				p_digit->state->directions.legs_count = 0;
			}
			if (p_digit->state->directions.legs_count > 0)
			{
				p_digit->state->directions.legs = (directions_leg_t *)nrf_malloc(sizeof(directions_leg_t) * p_digit->state->directions.legs_count);
				if (NULL == p_digit->state->directions.legs)
				{
					NRF_LOG_ERROR("Could not allocate legs memory");
					p_digit->state->directions.legs_count = 0;
				}
			}
			NRF_LOG_INFO("directions %d -> %d, %d legs", p_digit->state->directions.departure_time, p_digit->state->directions.arrival_time,
						 p_digit->state->directions.legs_count);
		}
		else
		{
			NRF_LOG_ERROR("Insufficient data length for directions: %d.", p_evt_write->len);
		}
	}
	if ((p_evt_write->handle == p_digit->directions_leg_char_handles.value_handle))
	{
		if (p_evt_write->len >= (CTS_CHAR_LENGTH + 5) || p_evt_write->len <= LEGS_CHAR_LENGTH)
		{
			uint8_t index = p_evt_write->data[0];
			if (index > (p_digit->state->directions.legs_count - 1))
			{
				NRF_LOG_ERROR("received leg %d for max %d legs", index, p_digit->state->directions.legs_count);
			}
			else
			{
				directions_leg_t *leg = p_digit->state->directions.legs + index;
				read_cts(p_evt_write->data + 1, &leg->departure_time);
				uint8_t line_size = p_evt_write->data[CTS_CHAR_LENGTH + 1];
				uint8_t direction_size = p_evt_write->data[CTS_CHAR_LENGTH + 2];
				uint8_t departure_size = p_evt_write->data[CTS_CHAR_LENGTH + 3];
				uint8_t arrival_size = p_evt_write->data[CTS_CHAR_LENGTH + 4];
				if (p_evt_write->len != (line_size + direction_size + departure_size + arrival_size + CTS_CHAR_LENGTH + 1 + 4))
				{
					NRF_LOG_ERROR("Insufficient text length for leg: %d.(%d,%d,%d,%d,%d)", p_evt_write->len,
								  line_size,
								  direction_size,
								  departure_size,
								  arrival_size, (CTS_CHAR_LENGTH + 1 + 4));
				}
				else
				{
					char *textPtr = (char *)p_evt_write->data + CTS_CHAR_LENGTH + 5;
					leg->line[0] = '\0';
					strncat(leg->line, textPtr, min(LINE_LENGTH - 1, line_size));
					textPtr += line_size;
					leg->direction[0] = '\0';
					strncat(leg->direction, textPtr, min(DIRECTION_LENGTH - 1, direction_size));
					textPtr += direction_size;
					leg->departure_stop[0] = '\0';
					strncat(leg->departure_stop, textPtr, min(STOP_LENGTH - 1, departure_size));
					textPtr += departure_size;
					leg->arrival_stop[0] = '\0';
					strncat(leg->arrival_stop, textPtr, min(STOP_LENGTH - 1, arrival_size));
					p_digit->state->directions.valid_legs |= (1 << index);
					NRF_LOG_INFO("leg %d at %d from %s to %s u %s/%s", index, leg->departure_time, leg->departure_stop, leg->arrival_stop, leg->line, leg->direction);
				}
			}
		}
		else
		{
			NRF_LOG_ERROR("Insufficient data length for leg: %d.", p_evt_write->len);
		}
	}
}

void ble_digit_on_ble_evt(ble_evt_t const *p_ble_evt, void *p_context)
{
	if ((p_context == NULL) || (p_ble_evt == NULL))
	{
		return;
	}

	ble_digit_t *p_digit = (ble_digit_t *)p_context;

	switch (p_ble_evt->header.evt_id)
	{
	case BLE_GATTS_EVT_WRITE:
		on_write(p_digit, p_ble_evt);
		break;

	default:
		break;
	}
}

static ret_code_t cts_char_add(ble_digit_t *p_digit, const ble_digit_init_t *p_digit_init)
{
	ret_code_t err_code;
	ble_add_char_params_t add_char_params;

	memset(&add_char_params, 0, sizeof(add_char_params));
	add_char_params.uuid_type = p_digit->uuid_type;
	add_char_params.uuid = DIGIT_UUID_CTS_CHAR;
	add_char_params.max_len = CTS_CHAR_LENGTH;
	add_char_params.init_len = 0;
	add_char_params.p_init_value = NULL;
	add_char_params.char_props.notify = false;
	add_char_params.char_props.read = 0;
	add_char_params.char_props.write = 1;
	add_char_params.cccd_write_access = SEC_NO_ACCESS;
	add_char_params.read_access = SEC_NO_ACCESS;
	add_char_params.write_access = SEC_JUST_WORKS;
	add_char_params.is_value_user = true;

	err_code = characteristic_add(p_digit->service_handle,
								  &add_char_params,
								  &(p_digit->cts_char_handles));
	return err_code;
}

static ret_code_t event_char_add(ble_digit_t *p_digit, const ble_digit_init_t *p_digit_init)
{
	ret_code_t err_code;
	ble_add_char_params_t add_char_params;

	memset(&add_char_params, 0, sizeof(add_char_params));
	add_char_params.uuid_type = p_digit->uuid_type;
	add_char_params.uuid = DIGIT_UUID_EVENT_CHAR;
	add_char_params.max_len = CTS_CHAR_LENGTH + SUBJECT_LENGTH;
	add_char_params.init_len = 0;
	add_char_params.p_init_value = NULL;
	add_char_params.char_props.notify = false;
	add_char_params.char_props.read = 0;
	add_char_params.char_props.write = 1;
	add_char_params.cccd_write_access = SEC_NO_ACCESS;
	add_char_params.read_access = SEC_NO_ACCESS;
	add_char_params.write_access = SEC_JUST_WORKS;
	add_char_params.is_value_user = true;

	err_code = characteristic_add(p_digit->service_handle,
								  &add_char_params,
								  &(p_digit->event_char_handles));
	return err_code;
}

static ret_code_t directions_char_add(ble_digit_t *p_digit, const ble_digit_init_t *p_digit_init)
{
	ret_code_t err_code;
	ble_add_char_params_t add_char_params;

	memset(&add_char_params, 0, sizeof(add_char_params));
	add_char_params.uuid_type = p_digit->uuid_type;
	add_char_params.uuid = DIGIT_UUID_DIRECTIONS_CHAR;
	add_char_params.max_len = CTS_CHAR_LENGTH * 2 + 1;
	add_char_params.init_len = 0;
	add_char_params.p_init_value = NULL;
	add_char_params.char_props.notify = false;
	add_char_params.char_props.read = 0;
	add_char_params.char_props.write = 1;
	add_char_params.cccd_write_access = SEC_NO_ACCESS;
	add_char_params.read_access = SEC_NO_ACCESS;
	add_char_params.write_access = SEC_JUST_WORKS;
	add_char_params.is_value_user = true;

	err_code = characteristic_add(p_digit->service_handle,
								  &add_char_params,
								  &(p_digit->directions_char_handles));
	return err_code;
}

static ret_code_t directions_leg_char_add(ble_digit_t *p_digit, const ble_digit_init_t *p_digit_init)
{
	ret_code_t err_code;
	ble_add_char_params_t add_char_params;

	memset(&add_char_params, 0, sizeof(add_char_params));
	add_char_params.uuid_type = p_digit->uuid_type;
	add_char_params.uuid = DIGIT_UUID_DIRECTIONS_LEG_CHAR;
	add_char_params.max_len = LEGS_CHAR_LENGTH;
	add_char_params.init_len = 0;
	add_char_params.p_init_value = NULL;
	add_char_params.char_props.notify = false;
	add_char_params.char_props.read = 0;
	add_char_params.char_props.write = 1;
	add_char_params.cccd_write_access = SEC_NO_ACCESS;
	add_char_params.read_access = SEC_NO_ACCESS;
	add_char_params.write_access = SEC_JUST_WORKS;
	add_char_params.is_value_user = true;

	err_code = characteristic_add(p_digit->service_handle,
								  &add_char_params,
								  &(p_digit->directions_leg_char_handles));
	return err_code;
}

ret_code_t ble_digit_init(ble_digit_t *p_digit, const ble_digit_init_t *p_digit_init)
{
	if (p_digit == NULL || p_digit_init == NULL || p_digit_init->state == NULL)
	{
		return NRF_ERROR_NULL;
	}
	p_digit->state = p_digit_init->state;
	ret_code_t err_code;
	ble_uuid_t ble_uuid;
	ble_uuid128_t base_uuid = {DIGIT_UUID_BASE};
	err_code = sd_ble_uuid_vs_add(&base_uuid, &p_digit->uuid_type);
	VERIFY_SUCCESS(err_code);
	ble_uuid.type = p_digit->uuid_type;
	ble_uuid.uuid = DIGIT_UUID_SERVICE;
	err_code = sd_ble_gatts_service_add(BLE_GATTS_SRVC_TYPE_PRIMARY, &ble_uuid, &p_digit->service_handle);
	VERIFY_SUCCESS(err_code);

	err_code = cts_char_add(p_digit, p_digit_init);
	VERIFY_SUCCESS(err_code);
	err_code = event_char_add(p_digit, p_digit_init);
	VERIFY_SUCCESS(err_code);
	err_code = directions_char_add(p_digit, p_digit_init);
	VERIFY_SUCCESS(err_code);
	err_code = directions_leg_char_add(p_digit, p_digit_init);
	VERIFY_SUCCESS(err_code);

	VERIFY_SUCCESS(nrf_mem_init());

	return NRF_SUCCESS;
}
