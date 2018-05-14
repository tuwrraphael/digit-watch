#include "nrf_drv_timer.h"
#include "nrf_drv_ppi.h"
#include "nrf_drv_gpiote.h"
#include "app_error.h"
#include "nrf_gpio.h"

#include "./display_extcomin.h"
#include "./display.h"

#define TIMER_INSTANCE 1 //TIMER 0 is reserved by softdevice
static nrf_drv_timer_t timer = NRF_DRV_TIMER_INSTANCE(TIMER_INSTANCE);

static void timer_dummy_handler(nrf_timer_event_t event_type, void * p_context) {}

static void timer_init() {
	nrf_drv_timer_config_t timer_cfg = NRF_DRV_TIMER_DEFAULT_CONFIG;
	timer_cfg.frequency = NRF_TIMER_FREQ_31250Hz;
	APP_ERROR_CHECK(nrf_drv_timer_init(&timer, &timer_cfg, timer_dummy_handler));
}

static void gpiote_task_init() {
	uint32_t compare_evt_addr;
	uint32_t gpiote_task_addr;
	nrf_ppi_channel_t ppi_channel;
	ret_code_t err_code;
	nrf_drv_gpiote_out_config_t config = GPIOTE_CONFIG_OUT_TASK_TOGGLE(false);

	err_code = nrf_drv_gpiote_out_init(DISPLAY_EXTCOMIN, &config);
	APP_ERROR_CHECK(err_code);


	nrf_drv_timer_extended_compare(&timer, (nrf_timer_cc_channel_t)0, 31250UL, NRF_TIMER_SHORT_COMPARE0_CLEAR_MASK, false);

	err_code = nrf_drv_ppi_channel_alloc(&ppi_channel);
	APP_ERROR_CHECK(err_code);

	compare_evt_addr = nrf_drv_timer_event_address_get(&timer, NRF_TIMER_EVENT_COMPARE0);
	gpiote_task_addr = nrf_drv_gpiote_out_task_addr_get(DISPLAY_EXTCOMIN);

	err_code = nrf_drv_ppi_channel_assign(ppi_channel, compare_evt_addr, gpiote_task_addr);
	APP_ERROR_CHECK(err_code);

	err_code = nrf_drv_ppi_channel_enable(ppi_channel);
	APP_ERROR_CHECK(err_code);
}

void extcomin_setup()
{
	nrf_gpio_cfg_output(DISPLAY_EXTCOMIN);
	APP_ERROR_CHECK(nrf_drv_ppi_init());
	if (!nrf_drv_gpiote_is_init())
	{
		APP_ERROR_CHECK(nrf_drv_gpiote_init());
	}
	timer_init();
}

void extcomin_enable() {
	gpiote_task_init();
	nrf_drv_gpiote_out_task_enable(DISPLAY_EXTCOMIN);
	nrf_drv_timer_enable(&timer);
}

void extcomin_disable() {
	nrf_drv_timer_disable(&timer);
	nrf_drv_gpiote_out_uninit(DISPLAY_EXTCOMIN);
	nrf_gpio_cfg_output(DISPLAY_EXTCOMIN);
	nrf_gpio_pin_write(DISPLAY_EXTCOMIN, 0);
}

