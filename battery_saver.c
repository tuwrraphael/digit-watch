#include "nrf_power.h"
#include "./battery_saver.h"
#include "./app_rtc.h"
#include "nrf_pwr_mgmt.h"

#define POWERSAVE_REG_VAL (7)

static bool check_powersave_mode()
{
    return nrf_power_gpregret_get() == POWERSAVE_REG_VAL;
}

static void shutdown_maintainance_callback()
{
}

bool start_battery_saver(void)
{
    bool enter = check_powersave_mode();
    if (enter)
    {
        NRF_POWER->DCDCEN = 1;
        nrf_power_gpregret_set(0);
        app_rtc_request();
        app_rtc_register_maintainance_callback(&shutdown_maintainance_callback);
        app_rtc_set_mode(APP_RTC_MODE_SHUTDOWN);
    }
    return enter;
}

void enter_battery_saver(void)
{
    nrf_power_gpregret_set(POWERSAVE_REG_VAL);
    nrf_pwr_mgmt_shutdown(NRF_PWR_MGMT_SHUTDOWN_RESET);
}