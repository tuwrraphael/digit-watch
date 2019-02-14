#include "nrfx_saadc.h"
#include "nrf_log.h"

#include "./battery.h"

#define BATTERY_LOW_THRESHOLD (739) // 2.6V * (1/6/0.6) * 2^10
#define BATTERY_CRITICAL_THRESHOLD (711) // 2.5V * (1/6/0.6) * 2^10
#define BATTERY_FULL (853) // 3V * (1/6/0.6) * 2^10
#define BATTERY_RANGE (BATTERY_FULL - BATTERY_LOW_THRESHOLD)
static const bool uninitialize = true; //stop easydma to save power

static nrf_saadc_value_t bms_buffer;
static battery_management_callback_t bms_callback;
static battery_state_t battery_state;
static bool initialized = false;

static void saadc_callback(nrfx_saadc_evt_t const *p_event)
{
	if (p_event->type == NRFX_SAADC_EVT_DONE)
	{
		ret_code_t err_code;
		if (uninitialize && initialized)
		{
			nrfx_saadc_uninit();
			initialized = false;
		}
		else
		{
			err_code = nrfx_saadc_buffer_convert(p_event->data.done.p_buffer, 1); //get ready for the next conversion when leaving initialized
			APP_ERROR_CHECK(err_code);
		}
		battery_state.value = *(p_event->data.done.p_buffer);
		battery_state.battery_level = battery_state.value < BATTERY_LOW_THRESHOLD ? (battery_state.value < BATTERY_CRITICAL_THRESHOLD ? BATTERY_LEVEL_CRITICAL : BATTERY_LEVEL_LOW) : BATTERY_LEVEL_OK;
		bms_callback(&battery_state);
	}
}

static ret_code_t init_adc(void)
{
	ret_code_t err_code;
	nrf_saadc_channel_config_t channel_config =
		NRFX_SAADC_DEFAULT_CHANNEL_CONFIG_SE(NRF_SAADC_INPUT_VDD);
	channel_config.burst = NRF_SAADC_BURST_ENABLED;
	channel_config.acq_time = NRF_SAADC_ACQTIME_20US;
	nrfx_saadc_config_t config = NRFX_SAADC_DEFAULT_CONFIG;
	err_code = nrfx_saadc_init(&config, saadc_callback);
	VERIFY_SUCCESS(err_code);
	err_code = nrfx_saadc_channel_init(0, &channel_config);
	VERIFY_SUCCESS(err_code);
	err_code = nrfx_saadc_buffer_convert(&bms_buffer, 1);
	VERIFY_SUCCESS(err_code);
	initialized = true;
	return err_code;
}

ret_code_t battery_management_init(battery_management_callback_t callback)
{
	if (NULL == callback)
	{
		return NRF_ERROR_INVALID_PARAM;
	}
	bms_callback = callback;
	return NRF_SUCCESS;
}

void battery_management_trigger(void)
{
	ret_code_t err_code;
	if (!initialized)
	{
		err_code = init_adc();
		APP_ERROR_CHECK(err_code);
	}
	err_code = nrfx_saadc_sample();
	APP_ERROR_CHECK(err_code);
}

uint8_t battery_level_percentage(battery_state_t *state) {
	uint8_t remaining = state->value - BATTERY_LOW_THRESHOLD;
	return (uint8_t)(((double)remaining/(double)BATTERY_RANGE) * 100);
}