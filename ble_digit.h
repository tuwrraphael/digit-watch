#ifndef APP_BLE_DIGIT_H
#define APP_BLE_DIGIT_H

#include <stdint.h>
#include <stdbool.h>
#include "ble.h"
#include "ble_srv_common.h"
#include "nrf_sdh_ble.h"

#ifdef __cplusplus
extern "C"
{
#endif

#define BLE_DIGIT_BLE_OBSERVER_PRIO 2

#define DIGIT_UUID_BASE                                                                                \
	{                                                                                                  \
		0x23, 0xD1, 0x13, 0xEF, 0x5F, 0x78, 0x23, 0x15, 0xDE, 0xEF, 0x12, 0x12, 0x00, 0x00, 0x00, 0x00 \
	}
#define DIGIT_UUID_SERVICE 0x1523
#define DIGIT_UUID_VALUE_CHAR 0x1524

#define BLE_DIGIT_DEF(_name)                          \
	static ble_digit_t _name;                         \
	NRF_SDH_BLE_OBSERVER(_name##_obs,                 \
						 BLE_DIGIT_BLE_OBSERVER_PRIO, \
						 ble_digit_on_ble_evt,        \
						 &_name)

	typedef struct ble_digit_s ble_digit_t;

	typedef struct
	{
	} ble_digit_init_t;

	struct ble_digit_s
	{
		uint16_t service_handle;
		ble_gatts_char_handles_t value_char_handles;
		uint8_t uuid_type;
	};

	ret_code_t ble_digit_init(ble_digit_t *p_digit, const ble_digit_init_t *p_digit_init);

	void ble_digit_on_ble_evt(ble_evt_t const *p_ble_evt, void *p_context);

#ifdef __cplusplus
}
#endif

#endif // APP_BLE_DIGIT_H
