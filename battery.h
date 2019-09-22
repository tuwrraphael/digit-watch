#ifndef APP_BATTERY_H
#define APP_BATTERY_H

typedef enum {
	BATTERY_LEVEL_OK,
	BATTERY_LEVEL_APPROACHING_LOW,
	BATTERY_LEVEL_LOW,
	BATTERY_LEVEL_CRITICAL
} battery_level_t;

typedef struct {
	uint16_t value;
	battery_level_t battery_level;
} battery_state_t;

typedef void(*battery_management_callback_t)(battery_state_t *battery_state);

ret_code_t battery_management_init(battery_management_callback_t callback);

void battery_management_trigger(void);

uint8_t battery_level_percentage(battery_state_t *battery_state);

#endif