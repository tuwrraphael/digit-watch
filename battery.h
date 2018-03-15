#ifndef APP_BATTERY_H
#define APP_BATTERY_H

typedef struct {
	uint16_t value;
	bool battery_low_warning;
} battery_state_t;

typedef void(*battery_management_callback_t)(battery_state_t *battery_state);

ret_code_t battery_management_init(battery_management_callback_t callback);

void battery_management_trigger(void);

#endif