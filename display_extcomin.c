#include "app_error.h"
#include "nrfx_gpiote.h"
#include "nrfx_ppi.h"
#include "nrfx_rtc.h"
#include "nrf_drv_clock.h"

#include "./display_extcomin.h"
#include "./display.h"

static nrfx_rtc_t rtc = NRFX_RTC_INSTANCE(2);
static bool enabled = false;

static void rtc_handler(nrfx_rtc_int_type_t int_type)
{
	if (NRFX_RTC_INT_COMPARE0 == int_type)
	{
		nrfx_rtc_counter_clear(&rtc);
		nrfx_rtc_int_enable(&rtc, NRF_RTC_INT_COMPARE0_MASK);
	}
}

static void rtc_config(void)
{
	uint32_t err_code;
	nrfx_rtc_config_t config = NRFX_RTC_DEFAULT_CONFIG;
	config.prescaler = RTC_FREQ_TO_PRESCALER(8);

	err_code = nrfx_rtc_init(&rtc, &config, rtc_handler);
	APP_ERROR_CHECK(err_code);

	err_code = nrfx_rtc_cc_set(&rtc, 0, 8, true);
	APP_ERROR_CHECK(err_code);
}

static void gpiote_task_init()
{
	uint32_t compare_evt_addr;
	uint32_t gpiote_task_addr;
	nrf_ppi_channel_t ppi_channel;
	ret_code_t err_code;
	nrfx_gpiote_out_config_t config = NRFX_GPIOTE_CONFIG_OUT_TASK_TOGGLE(false);

	err_code = nrfx_gpiote_out_init(DISPLAY_EXTCOMIN, &config);
	APP_ERROR_CHECK(err_code);

	err_code = nrfx_ppi_channel_alloc(&ppi_channel);
	APP_ERROR_CHECK(err_code);

	compare_evt_addr = nrfx_rtc_event_address_get(&rtc, NRF_RTC_EVENT_COMPARE_0);
	gpiote_task_addr = nrfx_gpiote_out_task_addr_get(DISPLAY_EXTCOMIN);

	err_code = nrfx_ppi_channel_assign(ppi_channel, compare_evt_addr, gpiote_task_addr);
	APP_ERROR_CHECK(err_code);

	err_code = nrfx_ppi_channel_enable(ppi_channel);
	APP_ERROR_CHECK(err_code);
}

void extcomin_setup()
{
	if (!nrfx_gpiote_is_init())
	{
		APP_ERROR_CHECK(nrfx_gpiote_init());
	}
	if (!nrf_drv_clock_init_check())
	{
		ret_code_t err_code = nrf_drv_clock_init();
		APP_ERROR_CHECK(err_code);
	}
	rtc_config();
}

void extcomin_enable()
{
	if (!enabled)
	{
		enabled = true;
		nrf_drv_clock_lfclk_request(NULL);
		gpiote_task_init();
		nrfx_gpiote_out_task_enable(DISPLAY_EXTCOMIN);
		nrfx_rtc_enable(&rtc);
	}
}

void extcomin_disable()
{
	if (enabled)
	{
		enabled = false;
		nrfx_rtc_disable(&rtc);
		nrfx_gpiote_out_uninit(DISPLAY_EXTCOMIN);
		nrfx_gpiote_out_config_t config = NRFX_GPIOTE_CONFIG_OUT_SIMPLE(false);
		nrfx_gpiote_out_init(DISPLAY_EXTCOMIN, &config);
		nrf_drv_clock_lfclk_release();
	}
}