#include "sdk_common.h"
#include "ble_bas.h"
#include <string.h>
#include "ble_srv_common.h"
#include "ble_conn_state.h"
#include "nrf_log.h"
#include "./ble_digit.h"

static void on_write(ble_digit_t *p_digit, ble_evt_t const *p_ble_evt)
{
	ble_gatts_evt_write_t const *p_evt_write = &p_ble_evt->evt.gatts_evt.params.write;
	if ((p_evt_write->handle == p_digit->value_char_handles.value_handle))
	{
		if (p_evt_write->len == 2)
		{
			NRF_LOG_INFO("got %d, %d", p_evt_write->data[0], p_evt_write->data[1]);
		}
		else
		{
			NRF_LOG_ERROR("Insufficient data length for cts: %d.", p_evt_write->len);
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

static ret_code_t value_char_add(ble_digit_t *p_digit, const ble_digit_init_t *p_digit_init)
{
	ret_code_t err_code;
	ble_add_char_params_t add_char_params;

	memset(&add_char_params, 0, sizeof(add_char_params));
	add_char_params.uuid_type = p_digit->uuid_type;
	add_char_params.uuid = DIGIT_UUID_VALUE_CHAR;
	add_char_params.max_len = 25; //TODO
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
								  &(p_digit->value_char_handles));
	return err_code;
}

ret_code_t ble_digit_init(ble_digit_t *p_digit, const ble_digit_init_t *p_digit_init)
{
	if (p_digit == NULL || p_digit_init == NULL)
	{
		return NRF_ERROR_NULL;
	}

	ret_code_t err_code;

	ble_uuid_t ble_uuid;
	ble_uuid128_t base_uuid = {DIGIT_UUID_BASE};
	err_code = sd_ble_uuid_vs_add(&base_uuid, &p_digit->uuid_type);
	VERIFY_SUCCESS(err_code);
	ble_uuid.type = p_digit->uuid_type;
	ble_uuid.uuid = DIGIT_UUID_SERVICE;
	err_code = sd_ble_gatts_service_add(BLE_GATTS_SRVC_TYPE_PRIMARY, &ble_uuid, &p_digit->service_handle);
	VERIFY_SUCCESS(err_code);

	err_code = value_char_add(p_digit, p_digit_init);
	VERIFY_SUCCESS(err_code);

	return NRF_SUCCESS;
}
