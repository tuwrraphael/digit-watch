#include "nrf_power.h"
#include "./battery_saver.h"
#include "./app_rtc.h"
#include "nrf_pwr_mgmt.h"
#include "./battery.h"
#include "nrf_log.h"
#include "./display.h"
#include "nrf_soc.h"
#include "nrf_gpio.h"
#include "digit_ui.h"

#define POWERSAVE_REG_VAL (1)

static bool check_powersave_mode()
{
    uint8_t gpregret_value = nrf_power_gpregret_get();
    return (gpregret_value & POWERSAVE_REG_VAL) > 0;
}

static void shutdown_maintainance_callback()
{
    battery_management_trigger();
}

static void battery_management_callback(battery_state_t *battery_state)
{
    switch (battery_state->battery_level)
    {
    case BATTERY_LEVEL_OK:
        app_rtc_release();
        nrf_power_gpregret_set(0);
        nrf_pwr_mgmt_shutdown(NRF_PWR_MGMT_SHUTDOWN_RESET);
        break;
    case BATTERY_LEVEL_CRITICAL:
        NRF_POWER->SYSTEMOFF = POWER_SYSTEMOFF_SYSTEMOFF_Enter;
        break;
    case BATTERY_LEVEL_LOW:
    case BATTERY_LEVEL_APPROACHING_LOW:
    default:
        break;
    }
}

bool start_battery_saver(void)
{
    bool enter = check_powersave_mode();
    if (enter)
    {
        NRF_POWER->DCDCEN = 1;
        nrf_power_gpregret_set(0);
        battery_management_init(&battery_management_callback);
        battery_management_trigger();
        app_rtc_request();
        app_rtc_register_maintainance_callback(&shutdown_maintainance_callback);
        app_rtc_set_mode(APP_RTC_MODE_SHUTDOWN);
        display_init_off();
    }
    return enter;
}

void enter_battery_saver(void)
{
    uint32_t gpregret_value = POWERSAVE_REG_VAL;
    nrf_power_gpregret_set(gpregret_value);
    nrf_pwr_mgmt_shutdown(NRF_PWR_MGMT_SHUTDOWN_RESET);
}