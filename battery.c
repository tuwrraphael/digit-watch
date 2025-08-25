#include "nrfx_saadc.h"
#include "nrf_log.h"

#include "./battery.h"

#define BATTERY_CRITICAL_THRESHOLD (711)		// 2.5V * (1/6/0.6) * 2^10 -> 0%
#define BATTERY_LOW_THRESHOLD (770)				// 2.60V * (1/6/0.6) * 2^10 -> 33%
#define BATTERY_APPROACHING_LOW_THRESHOLD (780) // 2.62V * (1/6/0.6) * 2^10 -> 38%
#define BATTERY_FULL (890)						// 3.13V * (1/6/0.6) * 2^10 -> 100%
#define BATTERY_RANGE (BATTERY_FULL - BATTERY_CRITICAL_THRESHOLD)

#define ARRAY_LEN(x) (sizeof(x) / sizeof(x[0]))

static const bool uninitialize = true; //stop easydma to save power

static nrf_saadc_value_t bms_buffer;
static battery_management_callback_t bms_callback;
static battery_state_t battery_state;
static bool initialized = false;

typedef struct
{
	battery_level_t level;
	uint16_t threshold;
} battery_map_element_t;

static battery_map_element_t battery_map[] = {
	{.threshold = BATTERY_CRITICAL_THRESHOLD, .level = BATTERY_LEVEL_CRITICAL},
	{.threshold = BATTERY_LOW_THRESHOLD, .level = BATTERY_LEVEL_LOW},
	{.threshold = BATTERY_APPROACHING_LOW_THRESHOLD, .level = BATTERY_LEVEL_APPROACHING_LOW},
};

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
		battery_state.battery_level = BATTERY_LEVEL_OK;
		for (uint8_t i = 0; i < ARRAY_LEN(battery_map); i++)
		{
			if (battery_state.value <= battery_map[i].threshold)
			{
				battery_state.battery_level = battery_map[i].level;
				break;
			}
		}
		// NRF_LOG_INFO("Battery measurement %u", battery_state.value);
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

uint8_t battery_level_percentage(battery_state_t *state)
{
	uint8_t remaining = state->value - BATTERY_CRITICAL_THRESHOLD;
	return (uint8_t)(((double)remaining / (double)BATTERY_RANGE) * 100);
}