#include "nrf_power.h"
#include "./battery_saver.h"
#include "./app_rtc.h"
#include "nrf_pwr_mgmt.h"
#include "./battery.h"
#include "nrf_log.h"
#include "./display.h"
#include "nrf_soc.h"

#define POWERSAVE_REG_VAL (7 << 8) //use byte 1

static bool check_powersave_mode()
{
    uint32_t gpregret_value;
    sd_power_gpregret_get(0, &gpregret_value);
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
        nrf_pwr_mgmt_shutdown(NRF_PWR_MGMT_SHUTDOWN_STAY_IN_SYSOFF);
        break;
    case BATTERY_LEVEL_LOW:
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
    sd_power_gpregret_set(0, gpregret_value);
    nrf_pwr_mgmt_shutdown(NRF_PWR_MGMT_SHUTDOWN_RESET);
}