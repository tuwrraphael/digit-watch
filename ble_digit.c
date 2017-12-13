#include "sdk_common.h"
#include <string.h>
#include "ble_srv_common.h"

#include "ble_digit.h"

#define INVALID_VALUE 255

static void on_connect(ble_digit_t *p_digit, ble_evt_t const * p_ble_evt)
{
	p_digit->conn_handle = p_ble_evt->evt.gap_evt.conn_handle;
}

static void on_disconnect(ble_digit_t * p_digit, ble_evt_t const * p_ble_evt)
{
	UNUSED_PARAMETER(p_ble_evt);
	p_digit->conn_handle = BLE_CONN_HANDLE_INVALID;
}

static void on_write(ble_digit_t * p_digit, ble_evt_t const * p_ble_evt)
{
	if (!p_digit->is_notification_supported)
	{
		return;
	}
	ble_gatts_evt_write_t const * p_evt_write = &p_ble_evt->evt.gatts_evt.params.write;

	if ((p_evt_write->handle == p_digit->value_char_handles.cccd_handle)
		&& (p_evt_write->len == 2))
	{
		return;
		//CCCD event handler stuff
	}
}


void ble_digit_on_ble_evt(ble_evt_t const * p_ble_evt, void * p_context)
{
	ble_digit_t *p_digit = (ble_digit_t *)p_context;

	switch (p_ble_evt->header.evt_id)
	{
	case BLE_GAP_EVT_CONNECTED:
		on_connect(p_digit, p_ble_evt);
		break;

	case BLE_GAP_EVT_DISCONNECTED:
		on_disconnect(p_digit, p_ble_evt);
		break;

	case BLE_GATTS_EVT_WRITE:
		on_write(p_digit, p_ble_evt);
		break;

	default:
		break;
	}
}

static uint32_t value_char_add(ble_digit_t * p_digit, const ble_digit_init_t * p_digit_init)
{
	ble_gatts_char_md_t char_md;
	ble_gatts_attr_md_t cccd_md;
	ble_gatts_attr_t    attr_char_value;

	ble_gatts_attr_md_t attr_md;
	uint8_t             initial_battery_level;
	ble_uuid_t			ble_uuid;

	if (p_digit->is_notification_supported)
	{
		memset(&cccd_md, 0, sizeof(cccd_md));
		BLE_GAP_CONN_SEC_MODE_SET_OPEN(&cccd_md.read_perm);
		BLE_GAP_CONN_SEC_MODE_SET_OPEN(&cccd_md.write_perm);
		cccd_md.vloc = BLE_GATTS_VLOC_STACK;
	}

	memset(&char_md, 0, sizeof(char_md));

	char_md.char_props.read = 1;
	char_md.char_props.notify = (p_digit->is_notification_supported) ? 1 : 0;
	char_md.p_char_user_desc = NULL;
	char_md.p_char_pf = NULL;
	char_md.p_user_desc_md = NULL;
	char_md.p_cccd_md = (p_digit->is_notification_supported) ? &cccd_md : NULL;
	char_md.p_sccd_md = NULL;

	ble_uuid.type = p_digit->uuid_type;
	ble_uuid.uuid = DIGIT_UUID_VALUE_CHAR;

	memset(&attr_md, 0, sizeof(attr_md));

	BLE_GAP_CONN_SEC_MODE_SET_OPEN(&attr_md.read_perm);
	BLE_GAP_CONN_SEC_MODE_SET_OPEN(&attr_md.write_perm);
	attr_md.vloc = BLE_GATTS_VLOC_STACK;
	attr_md.rd_auth = 0;
	attr_md.wr_auth = 0;
	attr_md.vlen = 0;

	initial_battery_level = p_digit_init->initial_value;

	memset(&attr_char_value, 0, sizeof(attr_char_value));

	attr_char_value.p_uuid = &ble_uuid;
	attr_char_value.p_attr_md = &attr_md;
	attr_char_value.init_len = sizeof(uint8_t);
	attr_char_value.init_offs = 0;
	attr_char_value.max_len = sizeof(uint8_t);
	attr_char_value.p_value = &initial_battery_level;

	return sd_ble_gatts_characteristic_add(p_digit->service_handle, 
		&char_md,
		&attr_char_value,
		&p_digit->value_char_handles);
}


uint32_t ble_digit_init(ble_digit_t * p_digit, const ble_digit_init_t * p_digit_init)
{
	if (p_digit == NULL || p_digit_init == NULL)
	{
		return NRF_ERROR_NULL;
	}

	uint32_t   err_code;
	ble_uuid_t ble_uuid;

	p_digit->conn_handle = BLE_CONN_HANDLE_INVALID;
	p_digit->is_notification_supported = p_digit_init->support_notification;
	p_digit->last_value = INVALID_VALUE;

	ble_uuid128_t base_uuid = { DIGIT_UUID_BASE };
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


uint32_t ble_digit_value_update(ble_digit_t * p_digit, uint8_t value)
{
	if (p_digit == NULL)
	{
		return NRF_ERROR_NULL;
	}

	uint32_t err_code = NRF_SUCCESS;
	ble_gatts_value_t gatts_value;

	if (value != p_digit->last_value)
	{
		memset(&gatts_value, 0, sizeof(gatts_value));

		gatts_value.len = sizeof(uint8_t);
		gatts_value.offset = 0;
		gatts_value.p_value = &value;

		err_code = sd_ble_gatts_value_set(p_digit->conn_handle,
			p_digit->value_char_handles.value_handle,
			&gatts_value);
		if (err_code == NRF_SUCCESS)
		{
			p_digit->last_value = value;
		}
		else
		{
			return err_code;
		}

		if ((p_digit->conn_handle != BLE_CONN_HANDLE_INVALID) && p_digit->is_notification_supported)
		{
			ble_gatts_hvx_params_t hvx_params;

			memset(&hvx_params, 0, sizeof(hvx_params));

			hvx_params.handle = p_digit->value_char_handles.value_handle;
			hvx_params.type = BLE_GATT_HVX_NOTIFICATION;
			hvx_params.offset = gatts_value.offset;
			hvx_params.p_len = &gatts_value.len;
			hvx_params.p_data = gatts_value.p_value;

			err_code = sd_ble_gatts_hvx(p_digit->conn_handle, &hvx_params);
		}
		else
		{
			err_code = NRF_ERROR_INVALID_STATE;
		}
	}

	return err_code;
}
