#ifndef APP_BLE_DIGIT_H
#define APP_BLE_DIGIT_H

#include <stdint.h>
#include <stdbool.h>
#include "ble.h"
#include "ble_srv_common.h"
#include "nrf_sdh_ble.h"
#include "digit_ui_model.h"

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
#define DIGIT_UUID_EVENT_CHAR 0x1524
#define DIGIT_UUID_DIRECTIONS_CHAR 0x1525
#define DIGIT_UUID_DIRECTIONS_LEG_CHAR 0x1526
#define DIGIT_UUID_CTS_CHAR 0x1805

#define BLE_DIGIT_DEF(_name)                          \
	static ble_digit_t _name;                         \
	NRF_SDH_BLE_OBSERVER(_name##_obs,                 \
						 BLE_DIGIT_BLE_OBSERVER_PRIO, \
						 ble_digit_on_ble_evt,        \
						 &_name)

	typedef struct ble_digit_s ble_digit_t;
	typedef void(*ui_state_changed_callback_t)(void);

	typedef struct
	{
		digit_ui_state_t *state;
		ui_state_changed_callback_t ui_state_changed;
	} ble_digit_init_t;

	struct ble_digit_s
	{
		uint16_t service_handle;
		ble_gatts_char_handles_t cts_char_handles;
		ble_gatts_char_handles_t event_char_handles;
		ble_gatts_char_handles_t directions_char_handles;
		ble_gatts_char_handles_t directions_leg_char_handles;
		uint8_t uuid_type;
		digit_ui_state_t *state;
		ui_state_changed_callback_t ui_state_changed;
	};

	ret_code_t ble_digit_init(ble_digit_t *p_digit, const ble_digit_init_t *p_digit_init);

	void ble_digit_on_ble_evt(ble_evt_t const *p_ble_evt, void *p_context);

#ifdef __cplusplus
}
#endif

#endif // APP_BLE_DIGIT_H
