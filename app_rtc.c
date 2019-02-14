#include "nrf.h"
#include "app_error.h"
#include "nrfx_rtc.h"
#include "nrf_drv_clock.h"
#include "nrf_sdh.h"

#include "./app_rtc.h"

#define APP_RTC_MODE_DEFAULT (APP_RTC_MODE_ACTIVE)
#define APP_RTC_TIME_1SEC (8)
#define APP_RTC_TIME_1MIN (60*APP_RTC_TIME_1SEC)
#define APP_RTC_TIME_15MIN (15*APP_RTC_TIME_1MIN)

static nrfx_rtc_t rtc = NRFX_RTC_INSTANCE(2);
static app_rtc_mode_t app_rtc_mode = APP_RTC_MODE_ACTIVE;
static app_rtc_shutdown_maintainance_callback_t mainainance_callback = NULL;
static uint8_t app_rtc_requests = 0;

static void rtc_handler(nrfx_rtc_int_type_t int_type)
{
    if (NRFX_RTC_INT_COMPARE0 == int_type)
    {
        nrfx_rtc_counter_clear(&rtc);
        nrfx_rtc_int_enable(&rtc, NRF_RTC_INT_COMPARE0_MASK);
        if (app_rtc_mode == APP_RTC_MODE_SHUTDOWN && NULL != mainainance_callback)
        {
            mainainance_callback();
        }
    }
}
static void app_rtc_init(void)
{
    uint32_t err_code;
    if (!nrf_drv_clock_init_check())
    {
        ret_code_t err_code = nrf_drv_clock_init();
        APP_ERROR_CHECK(err_code);
    }
    nrfx_rtc_config_t config = NRFX_RTC_DEFAULT_CONFIG;
    config.prescaler = RTC_FREQ_TO_PRESCALER(8);
    err_code = nrfx_rtc_init(&rtc, &config, rtc_handler);
    APP_ERROR_CHECK(err_code);
    app_rtc_set_mode(APP_RTC_MODE_DEFAULT);
    nrf_drv_clock_lfclk_request(NULL);
    nrfx_rtc_enable(&rtc);
}
static void app_rtc_uninit(void)
{
    nrfx_rtc_disable(&rtc);
    nrfx_rtc_uninit(&rtc);
    nrf_drv_clock_lfclk_release();
}
void app_rtc_register_maintainance_callback(app_rtc_shutdown_maintainance_callback_t callback)
{
    mainainance_callback = callback;
}
void app_rtc_set_mode(app_rtc_mode_t mode)
{
    if (app_rtc_requests == 0)
    {
        return;
    }
    uint32_t val = APP_RTC_TIME_1SEC;
    app_rtc_mode = mode;
    switch (mode)
    {
    case APP_RTC_MODE_ACTIVE:
        val = APP_RTC_TIME_1SEC;
        break;
    case APP_RTC_MODE_SHUTDOWN:
        val = APP_RTC_TIME_15MIN;
        break;
    }
    APP_ERROR_CHECK(nrfx_rtc_cc_set(&rtc, 0, val, true));
    nrfx_rtc_counter_clear(&rtc);
    nrfx_rtc_int_enable(&rtc, NRF_RTC_INT_COMPARE0_MASK);
}
void app_rtc_request(void)
{
    CRITICAL_REGION_ENTER();
    if (0 == app_rtc_requests)
    {
        app_rtc_init();
    }
    ++(app_rtc_requests);
    CRITICAL_REGION_EXIT();
}
void app_rtc_release(void)
{
    CRITICAL_REGION_ENTER();
    --(app_rtc_requests);
    if (app_rtc_requests == 0)
    {
        app_rtc_uninit();
    }
    CRITICAL_REGION_EXIT();
}
nrfx_rtc_t *app_rtc_get_instance(void)
{
    return &rtc;
}