#ifndef DIGIT_BUTTON_H
#define DIGIT_BUTTON_H

#include <stdint.h>
#include <stdbool.h>

typedef enum {
    BUTTON_EVENT_NONE = 0,
    BUTTON_EVENT_PRESSED,
    BUTTON_EVENT_RELEASED
} button_event_t;

typedef void (*button_event_handler_t)(button_event_t event);

void button_init(button_event_handler_t handler);

void button_deinit(void);

#endif // DIGIT_BUTTON_H
