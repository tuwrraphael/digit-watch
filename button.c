#include "button.h"
#include <nrf_gpio.h>
#include <nrf_drv_gpiote.h>
#include <app_error.h>

#define BUTTON_PIN  28

static button_event_handler_t m_button_handler = NULL;

static void button_gpiote_event_handler(nrf_drv_gpiote_pin_t pin, nrf_gpiote_polarity_t action) {
    if (m_button_handler == NULL) return;
    if (nrf_gpio_pin_read(BUTTON_PIN) == 0) {
        m_button_handler(BUTTON_EVENT_PRESSED);
    } else {
        m_button_handler(BUTTON_EVENT_RELEASED);
    }
}

void button_init(button_event_handler_t handler) {
    ret_code_t err_code;
    m_button_handler = handler;

    if (!nrf_drv_gpiote_is_init()) {
        err_code = nrf_drv_gpiote_init();
        APP_ERROR_CHECK(err_code);
    }

    nrf_drv_gpiote_in_config_t config = GPIOTE_CONFIG_IN_SENSE_TOGGLE(true);
    config.pull = NRF_GPIO_PIN_PULLUP;

    err_code = nrf_drv_gpiote_in_init(BUTTON_PIN, &config, button_gpiote_event_handler);
    APP_ERROR_CHECK(err_code);

    nrf_drv_gpiote_in_event_enable(BUTTON_PIN, true);
}

void button_deinit(void) {
    nrf_drv_gpiote_in_event_disable(BUTTON_PIN);
    nrf_drv_gpiote_in_uninit(BUTTON_PIN);
    m_button_handler = NULL;
}