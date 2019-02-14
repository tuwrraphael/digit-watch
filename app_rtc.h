#include "nrfx_rtc.h"

typedef enum
{
    APP_RTC_MODE_ACTIVE,
    APP_RTC_MODE_SHUTDOWN
} app_rtc_mode_t;

typedef void (*app_rtc_shutdown_maintainance_callback_t)(void);

void app_rtc_register_maintainance_callback(app_rtc_shutdown_maintainance_callback_t callback);

void app_rtc_set_mode(app_rtc_mode_t mode);

/**@brief requests the app rtc to run
 *
 * if it was previously uninitialized, it starts in APP_RTC_MODE_ACTIVE
 */
void app_rtc_request(void);

/**@brief indicates that the rtc is no longer needed by this module
 *
 * uninitializes the app rtc in case it is no longer needed
 */
void app_rtc_release(void);

nrfx_rtc_t *app_rtc_get_instance(void);