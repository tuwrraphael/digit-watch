#include "nrf_drv_saadc.h"
#include "nrf_log.h"

#include "./battery.h"

#define BATTERY_LOW_THRESHOLD (720)
static const bool uninitialize = true; //stop easydma to save power

static nrf_saadc_value_t     bms_buffer;
static battery_management_callback_t bms_callback;
static battery_state_t battery_state;
static bool initialized = false;

static void saadc_callback(nrf_drv_saadc_evt_t const * p_event)
{
	if (p_event->type == NRF_DRV_SAADC_EVT_DONE)
	{
		ret_code_t err_code;
		if (uninitialize && initialized) {
			nrf_drv_saadc_uninit();
			initialized = false;
		}
		else {
			err_code = nrf_drv_saadc_buffer_convert(p_event->data.done.p_buffer, 1);
			APP_ERROR_CHECK(err_code);
		}
		battery_state.value = *(p_event->data.done.p_buffer);
		battery_state.battery_low_warning = battery_state.value < BATTERY_LOW_THRESHOLD;
		bms_callback(&battery_state);
	}
}

static ret_code_t init_adc(void) {
	ret_code_t err_code;
	nrf_saadc_channel_config_t channel_config =
		NRF_DRV_SAADC_DEFAULT_CHANNEL_CONFIG_SE(NRF_SAADC_INPUT_VDD);
	channel_config.burst = NRF_SAADC_BURST_ENABLED;
	channel_config.acq_time = NRF_SAADC_ACQTIME_20US;
	err_code = nrf_drv_saadc_init(NULL, saadc_callback);
	VERIFY_SUCCESS(err_code);
	err_code = nrf_drv_saadc_channel_init(0, &channel_config);
	VERIFY_SUCCESS(err_code);
	err_code = nrf_drv_saadc_buffer_convert(&bms_buffer, 1);
	VERIFY_SUCCESS(err_code);
	initialized = true;
	return err_code;
}

ret_code_t battery_management_init(battery_management_callback_t callback)
{
	VERIFY_SUCCESS(init_adc());
	if (NULL == callback) {
		return NRF_ERROR_INVALID_PARAM;
	}
	bms_callback = callback;
	return NRF_SUCCESS;
}

void battery_management_trigger(void) {
	ret_code_t err_code;
	if (!initialized) {
		err_code = init_adc();
		APP_ERROR_CHECK(err_code);
	}
	err_code = nrf_drv_saadc_sample();
	APP_ERROR_CHECK(err_code);
}