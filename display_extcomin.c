#include "app_error.h"
#include "nrfx_gpiote.h"
#include "nrfx_ppi.h"
#include "nrfx_rtc.h"

#include "./display_extcomin.h"
#include "./display.h"
#include "./app_rtc.h"

static bool enabled = false;
static bool pin_in_use = false;

static void gpiote_task_init()
{
	uint32_t compare_evt_addr;
	uint32_t gpiote_task_addr;
	nrf_ppi_channel_t ppi_channel;
	ret_code_t err_code;
	nrfx_gpiote_out_config_t config = NRFX_GPIOTE_CONFIG_OUT_TASK_TOGGLE(false);

	if (pin_in_use)
	{
		nrfx_gpiote_out_uninit(DISPLAY_EXTCOMIN);
	}
	err_code = nrfx_gpiote_out_init(DISPLAY_EXTCOMIN, &config);
	pin_in_use = true;
	APP_ERROR_CHECK(err_code);

	err_code = nrfx_ppi_channel_alloc(&ppi_channel);
	APP_ERROR_CHECK(err_code);

	compare_evt_addr = nrfx_rtc_event_address_get(app_rtc_get_instance(), NRF_RTC_EVENT_COMPARE_0);
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
	app_rtc_request();
}

void extcomin_enable()
{
	if (!enabled)
	{
		enabled = true;
		gpiote_task_init();
		nrfx_gpiote_out_task_enable(DISPLAY_EXTCOMIN);
	}
}

void extcomin_disable()
{
	if (enabled)
	{
		enabled = false;
		if (pin_in_use)
		{
			nrfx_gpiote_out_uninit(DISPLAY_EXTCOMIN);
		}
		nrfx_gpiote_out_config_t config = NRFX_GPIOTE_CONFIG_OUT_SIMPLE(false);
		APP_ERROR_CHECK(nrfx_gpiote_out_init(DISPLAY_EXTCOMIN, &config));
		pin_in_use = true;
	}
}

void extcomin_uninit()
{
	extcomin_disable();
	app_rtc_release();
}