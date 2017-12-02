#include "nrf_drv_timer.h"
#include "nrf_drv_ppi.h"
#include "nrf_drv_gpiote.h"
#include "app_error.h"
#include "nrf_gpio.h"

#include "./display_extcomin.h"
#include "./display.h"

#define TIMER_INSTACE 1 //TIMER 0 is reserved by softdevice
static nrf_drv_timer_t timer = NRF_DRV_TIMER_INSTANCE(TIMER_INSTACE);

static void timer_dummy_handler(nrf_timer_event_t event_type, void * p_context) {}

void extcomin_setup()
{
	nrf_gpio_cfg_output(DISPLAY_EXTCOMIN);

	APP_ERROR_CHECK(nrf_drv_ppi_init());

	//err_code = nrf_drv_gpiote_init();// this is already initialized by the app_button library
	//APP_ERROR_CHECK(err_code);

	nrf_drv_timer_config_t timer_cfg = NRF_DRV_TIMER_DEFAULT_CONFIG;
	APP_ERROR_CHECK(nrf_drv_timer_init(&timer, &timer_cfg, timer_dummy_handler));

	uint32_t compare_evt_addr;
	uint32_t gpiote_task_addr;
	nrf_ppi_channel_t ppi_channel;
	ret_code_t err_code;
	nrf_drv_gpiote_out_config_t config = GPIOTE_CONFIG_OUT_TASK_TOGGLE(false);

	err_code = nrf_drv_gpiote_out_init(DISPLAY_EXTCOMIN, &config);
	APP_ERROR_CHECK(err_code);


	nrf_drv_timer_extended_compare(&timer, (nrf_timer_cc_channel_t)0, 1000 /* ms */ * 1000UL, NRF_TIMER_SHORT_COMPARE0_CLEAR_MASK, false);

	err_code = nrf_drv_ppi_channel_alloc(&ppi_channel);
	APP_ERROR_CHECK(err_code);

	compare_evt_addr = nrf_drv_timer_event_address_get(&timer, NRF_TIMER_EVENT_COMPARE0);
	gpiote_task_addr = nrf_drv_gpiote_out_task_addr_get(DISPLAY_EXTCOMIN);

	err_code = nrf_drv_ppi_channel_assign(ppi_channel, compare_evt_addr, gpiote_task_addr);
	APP_ERROR_CHECK(err_code);

	err_code = nrf_drv_ppi_channel_enable(ppi_channel);
	APP_ERROR_CHECK(err_code);

	nrf_drv_gpiote_out_task_enable(DISPLAY_EXTCOMIN);
	nrf_drv_timer_enable(&timer);
}