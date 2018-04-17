#include <stdint.h>
#include <string.h>
#include "nordic_common.h"
#include "nrf.h"
#include "nrf_sdm.h"
#include "app_error.h"
#include "ble.h"
#include "ble_err.h"
#include "ble_hci.h"
#include "ble_srv_common.h"
#include "ble_advdata.h"
#include "ble_advertising.h"
#include "ble_bas.h"
#include "ble_dis.h"
#include "ble_conn_params.h"
#include "nrf_sdh.h"
#include "nrf_sdh_ble.h"
#include "nrf_sdh_soc.h"
#include "app_timer.h"
#include "bsp_btn_ble.h"
#include "peer_manager.h"
#include "fds.h"
#include "nrf_ble_gatt.h"
#include "ble_conn_state.h"

#include "nrf_log.h"
#include "nrf_log_ctrl.h"
#include "nrf_log_default_backends.h"

#include "nrf_dfu_svci.h"
#include "nrf_svci_async_function.h"
#include "nrf_svci_async_handler.h"
#include "ble_dfu.h"
#include "nrf_pwr_mgmt.h"

#include "./device_information.h"
#include "./display.h"
#include "./ble_digit.h"
#include "./time_state.h"
#include "./battery.h"

#define APP_ADV_INTERVAL                    300                                     /**< The advertising interval (in units of 0.625 ms. This value corresponds to 187.5 ms). */
#define APP_ADV_TIMEOUT_IN_SECONDS          180                                     /**< The advertising timeout in units of seconds. */

#define APP_BLE_CONN_CFG_TAG                1                                       /**< A tag identifying the SoftDevice BLE configuration. */
#define APP_BLE_OBSERVER_PRIO               3                                       /**< Application's BLE observer priority. You shouldn't need to modify this value. */

#define BATTERY_LEVEL_MEAS_INTERVAL         APP_TIMER_TICKS(300000)
#define CT_TIMER_INTERVAL                   APP_TIMER_TICKS(1000)

#define MIN_CONN_INTERVAL                   MSEC_TO_UNITS(400, UNIT_1_25_MS)        /**< Minimum acceptable connection interval (0.4 seconds). */
#define MAX_CONN_INTERVAL                   MSEC_TO_UNITS(650, UNIT_1_25_MS)        /**< Maximum acceptable connection interval (0.65 second). */
#define SLAVE_LATENCY                       0                                       /**< Slave latency. */
#define CONN_SUP_TIMEOUT                    MSEC_TO_UNITS(4000, UNIT_10_MS)         /**< Connection supervisory timeout (4 seconds). */

#define FIRST_CONN_PARAMS_UPDATE_DELAY      APP_TIMER_TICKS(5000)                   /**< Time from initiating event (connect or start of notification) to first time sd_ble_gap_conn_param_update is called (5 seconds). */
#define NEXT_CONN_PARAMS_UPDATE_DELAY       APP_TIMER_TICKS(30000)                  /**< Time between each call to sd_ble_gap_conn_param_update after the first call (30 seconds). */
#define MAX_CONN_PARAMS_UPDATE_COUNT        3                                       /**< Number of attempts before giving up the connection parameter negotiation. */

#define SEC_PARAM_BOND                      1                                       /**< Perform bonding. */
#define SEC_PARAM_MITM                      0                                       /**< Man In The Middle protection not required. */
#define SEC_PARAM_LESC                      0                                       /**< LE Secure Connections not enabled. */
#define SEC_PARAM_KEYPRESS                  0                                       /**< Keypress notifications not enabled. */
#define SEC_PARAM_IO_CAPABILITIES           BLE_GAP_IO_CAPS_NONE                    /**< No I/O capabilities. */
#define SEC_PARAM_OOB                       0                                       /**< Out Of Band data not available. */
#define SEC_PARAM_MIN_KEY_SIZE              7                                       /**< Minimum encryption key size. */
#define SEC_PARAM_MAX_KEY_SIZE              16                                      /**< Maximum encryption key size. */

#define DEAD_BEEF                           0xDEADBEEF                              /**< Value used as error code on stack dump, can be used to identify stack location on stack unwind. */

#define APP_FEATURE_NOT_SUPPORTED           BLE_GATT_STATUS_ATTERR_APP_BEGIN + 2    /**< Reply when unsupported features are requested. */

BLE_BAS_DEF(m_bas);                                                 /**< Structure used to identify the battery service. */
BLE_DIGIT_DEF(m_digit);
NRF_BLE_GATT_DEF(m_gatt);                                           /**< GATT module instance. */
BLE_ADVERTISING_DEF(m_advertising);                                 /**< Advertising module instance. */
APP_TIMER_DEF(m_battery_timer_id);
APP_TIMER_DEF(ct_timer_id);

static uint16_t m_conn_handle = BLE_CONN_HANDLE_INVALID;    /**< Handle of the current connection. */

static time_state_t time_state;


/**@brief Callback function for asserts in the SoftDevice.
 *
 * @details This function will be called in case of an assert in the SoftDevice.
 *
 * @warning This handler is an example only and does not fit a final product. You need to analyze
 *          how your product is supposed to react in case of Assert.
 * @warning On assert from the SoftDevice, the system can only recover on reset.
 *
 * @param[in] line_num   Line number of the failing ASSERT call.
 * @param[in] file_name  File name of the failing ASSERT call.
 */
void assert_nrf_callback(uint16_t line_num, const uint8_t * p_file_name)
{
	app_error_handler(DEAD_BEEF, line_num, p_file_name);
}

/**@brief Handler for shutdown preparation.
*
* @details During shutdown procedures, this function will be called at a 1 second interval
*          untill the function returns true. When the function returns true, it means that the
*          app is ready to reset to DFU mode.
*
* @param[in]   event   Power manager event.
*
* @retval  True if shutdown is allowed by this power manager handler, otherwise false.
*/
static bool app_shutdown_handler(nrf_pwr_mgmt_evt_t event)
{
	switch (event)
	{
	case NRF_PWR_MGMT_EVT_PREPARE_DFU:
		NRF_LOG_INFO("Power management wants to reset to DFU mode.");
		// YOUR_JOB: Get ready to reset into DFU mode
		//
		// If you aren't finished with any ongoing tasks, return "false" to
		// signal to the system that reset is impossible at this stage.
		//
		// Here is an example using a variable to delay resetting the device.
		//
		// if (!m_ready_for_reset)
		// {
		//      return false;
		// }
		// else
		//{
		//
		//    // Device ready to enter
		//    uint32_t err_code;
		//    err_code = sd_softdevice_disable();
		//    APP_ERROR_CHECK(err_code);
		//    err_code = app_timer_stop_all();
		//    APP_ERROR_CHECK(err_code);
		//}
		break;

	default:
		// YOUR_JOB: Implement any of the other events available from the power management module:
		//      -NRF_PWR_MGMT_EVT_PREPARE_SYSOFF
		//      -NRF_PWR_MGMT_EVT_PREPARE_WAKEUP
		//      -NRF_PWR_MGMT_EVT_PREPARE_RESET
		return true;
	}

	NRF_LOG_INFO("Power management allowed to reset to DFU mode.");
	return true;
}
NRF_PWR_MGMT_HANDLER_REGISTER(app_shutdown_handler, 0);



/**@brief Clear bond information from persistent storage.
 */
static void delete_bonds(void)
{
	ret_code_t err_code;

	NRF_LOG_INFO("Erase bonds!");

	err_code = pm_peers_delete();
	APP_ERROR_CHECK(err_code);
}


/**@brief Function for starting advertising.
 */
void advertising_start(bool erase_bonds)
{
	if (erase_bonds == true)
	{
		delete_bonds();
		// Advertising is started by PM_EVT_PEERS_DELETE_SUCCEEDED event.
	}
	else
	{
		ret_code_t err_code;

		err_code = ble_advertising_start(&m_advertising, BLE_ADV_MODE_FAST);
		APP_ERROR_CHECK(err_code);
	}
}


/**@brief Function for handling File Data Storage events.
 *
 * @param[in] p_evt  Peer Manager event.
 * @param[in] cmd
 */
static void fds_evt_handler(fds_evt_t const * const p_evt)
{
	if (p_evt->id == FDS_EVT_GC)
	{
		NRF_LOG_DEBUG("GC completed\n");
	}
}


/**@brief Function for handling Peer Manager events.
 *
 * @param[in] p_evt  Peer Manager event.
 */
static void pm_evt_handler(pm_evt_t const * p_evt)
{
	ret_code_t err_code;

	switch (p_evt->evt_id)
	{
	case PM_EVT_BONDED_PEER_CONNECTED:
	{
		NRF_LOG_INFO("Connected to a previously bonded device.");
	} break;

	case PM_EVT_CONN_SEC_SUCCEEDED:
	{
		NRF_LOG_INFO("Connection secured: role: %d, conn_handle: 0x%x, procedure: %d.",
			ble_conn_state_role(p_evt->conn_handle),
			p_evt->conn_handle,
			p_evt->params.conn_sec_succeeded.procedure);
	} break;

	case PM_EVT_CONN_SEC_FAILED:
	{
		/* Often, when securing fails, it shouldn't be restarted, for security reasons.
		 * Other times, it can be restarted directly.
		 * Sometimes it can be restarted, but only after changing some Security Parameters.
		 * Sometimes, it cannot be restarted until the link is disconnected and reconnected.
		 * Sometimes it is impossible, to secure the link, or the peer device does not support it.
		 * How to handle this error is highly application dependent. */
	} break;

	case PM_EVT_CONN_SEC_CONFIG_REQ:
	{
		// Reject pairing request from an already bonded peer.
		pm_conn_sec_config_t conn_sec_config = { .allow_repairing = false };
		pm_conn_sec_config_reply(p_evt->conn_handle, &conn_sec_config);
	} break;

	case PM_EVT_STORAGE_FULL:
	{
		// Run garbage collection on the flash.
		err_code = fds_gc();
		if (err_code == FDS_ERR_BUSY || err_code == FDS_ERR_NO_SPACE_IN_QUEUES)
		{
			// Retry.
		}
		else
		{
			APP_ERROR_CHECK(err_code);
		}
	} break;

	case PM_EVT_PEERS_DELETE_SUCCEEDED:
	{
		NRF_LOG_DEBUG("PM_EVT_PEERS_DELETE_SUCCEEDED");
		advertising_start(false);
	} break;

	case PM_EVT_LOCAL_DB_CACHE_APPLY_FAILED:
	{
		// The local database has likely changed, send service changed indications.
		pm_local_database_has_changed();
	} break;

	case PM_EVT_PEER_DATA_UPDATE_FAILED:
	{
		// Assert.
		APP_ERROR_CHECK(p_evt->params.peer_data_update_failed.error);
	} break;

	case PM_EVT_PEER_DELETE_FAILED:
	{
		// Assert.
		APP_ERROR_CHECK(p_evt->params.peer_delete_failed.error);
	} break;

	case PM_EVT_PEERS_DELETE_FAILED:
	{
		// Assert.
		APP_ERROR_CHECK(p_evt->params.peers_delete_failed_evt.error);
	} break;

	case PM_EVT_ERROR_UNEXPECTED:
	{
		// Assert.
		APP_ERROR_CHECK(p_evt->params.error_unexpected.error);
	} break;

	case PM_EVT_CONN_SEC_START:
	case PM_EVT_PEER_DATA_UPDATE_SUCCEEDED:
	case PM_EVT_PEER_DELETE_SUCCEEDED:
	case PM_EVT_LOCAL_DB_CACHE_APPLIED:
	case PM_EVT_SERVICE_CHANGED_IND_SENT:
	case PM_EVT_SERVICE_CHANGED_IND_CONFIRMED:
	default:
		break;
	}
}

static void power_off() {
	app_timer_stop(ct_timer_id);
	app_timer_stop(m_battery_timer_id);
	display_uninit();
	nrf_pwr_mgmt_shutdown(NRF_PWR_MGMT_SHUTDOWN_STAY_IN_SYSOFF);
}

static void battery_level_meas_timeout_handler(void * p_context)
{
	UNUSED_PARAMETER(p_context);
	battery_management_trigger();
}

static void ct_timeout_handler(void * p_context)
{
	UNUSED_PARAMETER(p_context);
	if (time_state.time_known) {
		uint16_t hourDisplay = (time_state.cts_date.hour % 12);
		hourDisplay = ((time_state.cts_date.minute * 5) / 60) + (time_state.cts_date.hour % 12) * 5;
		draw_time_indicator(time_state.cts_date.second, 60, 1);
		draw_time_indicator(time_state.cts_date.minute, 50, 2);
		draw_time_indicator(hourDisplay, 35, 2);

		transfer_buffer_to_display();

		switch_display_mode();
		if (time_state.cts_date.second == 59) {
			if (time_state.cts_date.minute == 59) {
				time_state.cts_date.hour = (time_state.cts_date.hour + 1) % 24;
			}
			time_state.cts_date.minute = (time_state.cts_date.minute + 1) % 60;
		}
		time_state.cts_date.second = (time_state.cts_date.second + 1) % 60;
	}
}

static void timers_init(void)
{
	ret_code_t err_code;

	err_code = app_timer_init();
	APP_ERROR_CHECK(err_code);

	err_code = app_timer_create(&m_battery_timer_id,
		APP_TIMER_MODE_REPEATED,
		battery_level_meas_timeout_handler);
	APP_ERROR_CHECK(err_code);

	err_code = app_timer_create(&ct_timer_id,
		APP_TIMER_MODE_REPEATED,
		ct_timeout_handler);
	APP_ERROR_CHECK(err_code);
}


/**@brief Function for the GAP initialization.
 *
 * @details This function sets up all the necessary GAP (Generic Access Profile) parameters of the
 *          device including the device name, appearance, and the preferred connection parameters.
 */
static void gap_params_init(void)
{
	ret_code_t              err_code;
	ble_gap_conn_params_t   gap_conn_params;
	ble_gap_conn_sec_mode_t sec_mode;

	BLE_GAP_CONN_SEC_MODE_SET_OPEN(&sec_mode);

	err_code = sd_ble_gap_device_name_set(&sec_mode,
		(const uint8_t *)DEVICE_NAME,
		strlen(DEVICE_NAME));
	APP_ERROR_CHECK(err_code);

	err_code = sd_ble_gap_appearance_set(BLE_APPEARANCE_GENERIC_WATCH);
	APP_ERROR_CHECK(err_code);

	memset(&gap_conn_params, 0, sizeof(gap_conn_params));

	gap_conn_params.min_conn_interval = MIN_CONN_INTERVAL;
	gap_conn_params.max_conn_interval = MAX_CONN_INTERVAL;
	gap_conn_params.slave_latency = SLAVE_LATENCY;
	gap_conn_params.conn_sup_timeout = CONN_SUP_TIMEOUT;

	err_code = sd_ble_gap_ppcp_set(&gap_conn_params);
	APP_ERROR_CHECK(err_code);
}


/**@brief GATT module event handler.
 */
static void gatt_evt_handler(nrf_ble_gatt_t * p_gatt, nrf_ble_gatt_evt_t const * p_evt)
{
	if (p_evt->evt_id == NRF_BLE_GATT_EVT_ATT_MTU_UPDATED)
	{
		NRF_LOG_INFO("GATT ATT MTU on connection 0x%x changed to %d.",
			p_evt->conn_handle,
			p_evt->params.att_mtu_effective);
	}
}


/**@brief Function for initializing the GATT module.
 */
static void gatt_init(void)
{
	ret_code_t err_code = nrf_ble_gatt_init(&m_gatt, gatt_evt_handler);
	APP_ERROR_CHECK(err_code);
}

static void create_adv_data_packet(ble_advdata_t *advdata) {
	ble_uuid_t digit_service_uuid;
	digit_service_uuid.uuid = DIGIT_UUID_SERVICE;
	digit_service_uuid.type = m_digit.uuid_type;

	ble_uuid_t m_adv_uuids[] =
	{
		digit_service_uuid
	};

	uint8_t data[] = { time_state.time_known };
	ble_advdata_manuf_data_t manuf_specific_data;
	manuf_specific_data.company_identifier = 89;
	manuf_specific_data.data.p_data = data;
	manuf_specific_data.data.size = sizeof(data);

	advdata->name_type = BLE_ADVDATA_FULL_NAME;
	advdata->include_appearance = true;
	advdata->flags = BLE_GAP_ADV_FLAGS_LE_ONLY_GENERAL_DISC_MODE;
	advdata->uuids_complete.uuid_cnt = sizeof(m_adv_uuids) / sizeof(m_adv_uuids[0]);
	advdata->uuids_complete.p_uuids = m_adv_uuids;
	advdata->p_manuf_specific_data = &manuf_specific_data;
}


static void cts_received(cts_date_t *date) {
	memcpy(&time_state.cts_date, date, sizeof(cts_date_t));
	NRF_LOG_INFO("cts received, %d, %d, %d", date->hour, date->minute, date->second);
	time_state.time_known = true;
	ble_advdata_t advdata;
	create_adv_data_packet(&advdata);
	ble_advdata_set(&advdata, NULL);
}

// YOUR_JOB: Update this code if you want to do anything given a DFU event (optional).
/**@brief Function for handling dfu events from the Buttonless Secure DFU service
*
* @param[in]   event   Event from the Buttonless Secure DFU service.
*/
static void ble_dfu_evt_handler(ble_dfu_buttonless_evt_type_t event)
{
	switch (event)
	{
	case BLE_DFU_EVT_BOOTLOADER_ENTER_PREPARE:
		NRF_LOG_INFO("Device is preparing to enter bootloader mode.");
		// YOUR_JOB: Disconnect all bonded devices that currently are connected.
		//           This is required to receive a service changed indication
		//           on bootup after a successful (or aborted) Device Firmware Update.
		break;

	case BLE_DFU_EVT_BOOTLOADER_ENTER:
		// YOUR_JOB: Write app-specific unwritten data to FLASH, control finalization of this
		//           by delaying reset by reporting false in app_shutdown_handler
		NRF_LOG_INFO("Device will enter bootloader mode.");
		break;

	case BLE_DFU_EVT_BOOTLOADER_ENTER_FAILED:
		NRF_LOG_ERROR("Request to enter bootloader mode failed asynchroneously.");
		// YOUR_JOB: Take corrective measures to resolve the issue
		//           like calling APP_ERROR_CHECK to reset the device.
		break;

	case BLE_DFU_EVT_RESPONSE_SEND_ERROR:
		NRF_LOG_ERROR("Request to send a response to client failed.");
		// YOUR_JOB: Take corrective measures to resolve the issue
		//           like calling APP_ERROR_CHECK to reset the device.
		APP_ERROR_CHECK(false);
		break;

	default:
		NRF_LOG_ERROR("Unknown event from ble_dfu_buttonless.");
		break;
	}
}

static void services_init(void)
{
	ret_code_t     err_code;
	ble_bas_init_t bas_init;
	ble_dis_init_t dis_init;
	ble_digit_init_t digit_init;

	// Initialize Battery Service.
	memset(&bas_init, 0, sizeof(bas_init));

	// Here the sec level for the Battery Service can be changed/increased.
	BLE_GAP_CONN_SEC_MODE_SET_OPEN(&bas_init.battery_level_char_attr_md.cccd_write_perm);
	BLE_GAP_CONN_SEC_MODE_SET_OPEN(&bas_init.battery_level_char_attr_md.read_perm);
	BLE_GAP_CONN_SEC_MODE_SET_NO_ACCESS(&bas_init.battery_level_char_attr_md.write_perm);

	BLE_GAP_CONN_SEC_MODE_SET_OPEN(&bas_init.battery_level_report_read_perm);

	bas_init.evt_handler = NULL;
	bas_init.support_notification = false;
	bas_init.p_report_ref = NULL;
	bas_init.initial_batt_level = 100;

	err_code = ble_bas_init(&m_bas, &bas_init);
	APP_ERROR_CHECK(err_code);

	memset(&digit_init, 0, sizeof(digit_init));

	digit_init.initial_value = 100;
	digit_init.cts_received = cts_received;

	err_code = ble_digit_init(&m_digit, &digit_init);
	APP_ERROR_CHECK(err_code);

	// Initialize Device Information Service.
	memset(&dis_init, 0, sizeof(dis_init));

	ble_srv_ascii_to_utf8(&dis_init.manufact_name_str, (char *)MANUFACTURER_NAME);

	BLE_GAP_CONN_SEC_MODE_SET_OPEN(&dis_init.dis_attr_md.read_perm);
	BLE_GAP_CONN_SEC_MODE_SET_NO_ACCESS(&dis_init.dis_attr_md.write_perm);

	err_code = ble_dis_init(&dis_init);
	APP_ERROR_CHECK(err_code);

	ble_dfu_buttonless_init_t dfus_init =
	{
		.evt_handler = ble_dfu_evt_handler
	};

	// Initialize the async SVCI interface to bootloader.
	err_code = ble_dfu_buttonless_async_svci_init();
	APP_ERROR_CHECK(err_code);


	err_code = ble_dfu_buttonless_init(&dfus_init);
	APP_ERROR_CHECK(err_code);
}




/**@brief Function for starting application timers.
 */
static void application_timers_start(void)
{
	ret_code_t err_code;

	// Start application timers.
	err_code = app_timer_start(m_battery_timer_id, BATTERY_LEVEL_MEAS_INTERVAL, NULL);
	APP_ERROR_CHECK(err_code);

	err_code = app_timer_start(ct_timer_id, CT_TIMER_INTERVAL, NULL);
	APP_ERROR_CHECK(err_code);
}


/**@brief Function for handling the Connection Parameters Module.
 *
 * @details This function will be called for all events in the Connection Parameters Module which
 *          are passed to the application.
 *          @note All this function does is to disconnect. This could have been done by simply
 *                setting the disconnect_on_fail config parameter, but instead we use the event
 *                handler mechanism to demonstrate its use.
 *
 * @param[in] p_evt  Event received from the Connection Parameters Module.
 */
static void on_conn_params_evt(ble_conn_params_evt_t * p_evt)
{
	ret_code_t err_code;

	if (p_evt->evt_type == BLE_CONN_PARAMS_EVT_FAILED)
	{
		err_code = sd_ble_gap_disconnect(m_conn_handle, BLE_HCI_CONN_INTERVAL_UNACCEPTABLE);
		APP_ERROR_CHECK(err_code);
	}
}


/**@brief Function for handling a Connection Parameters error.
 *
 * @param[in] nrf_error  Error code containing information about what went wrong.
 */
static void conn_params_error_handler(uint32_t nrf_error)
{
	APP_ERROR_HANDLER(nrf_error);
}


/**@brief Function for initializing the Connection Parameters module.
 */
static void conn_params_init(void)
{
	ret_code_t             err_code;
	ble_conn_params_init_t cp_init;

	memset(&cp_init, 0, sizeof(cp_init));

	cp_init.p_conn_params = NULL;
	cp_init.first_conn_params_update_delay = FIRST_CONN_PARAMS_UPDATE_DELAY;
	cp_init.next_conn_params_update_delay = NEXT_CONN_PARAMS_UPDATE_DELAY;
	cp_init.max_conn_params_update_count = MAX_CONN_PARAMS_UPDATE_COUNT;
	cp_init.start_on_notify_cccd_handle = BLE_GATT_HANDLE_INVALID;
	cp_init.disconnect_on_fail = false;
	cp_init.evt_handler = on_conn_params_evt;
	cp_init.error_handler = conn_params_error_handler;

	err_code = ble_conn_params_init(&cp_init);
	APP_ERROR_CHECK(err_code);
}

/**@brief Function for handling advertising events.
 *
 * @details This function will be called for advertising events which are passed to the application.
 *
 * @param[in] ble_adv_evt  Advertising event.
 */
static void on_adv_evt(ble_adv_evt_t ble_adv_evt)
{
	switch (ble_adv_evt)
	{
	case BLE_ADV_EVT_FAST:
		NRF_LOG_INFO("Fast advertising.");
		break;

	case BLE_ADV_EVT_IDLE:
		NRF_LOG_INFO("Advertising idle.");
		break;

	default:
		break;
	}
}


/**@brief Function for handling BLE events.
 *
 * @param[in]   p_ble_evt   Bluetooth stack event.
 * @param[in]   p_context   Unused.
 */
static void ble_evt_handler(ble_evt_t const * p_ble_evt, void * p_context)
{
	ret_code_t err_code;

	switch (p_ble_evt->header.evt_id)
	{
	case BLE_GAP_EVT_CONNECTED:
		NRF_LOG_INFO("Connected.");
		m_conn_handle = p_ble_evt->evt.gap_evt.conn_handle;
		break;

	case BLE_GAP_EVT_DISCONNECTED:
		NRF_LOG_INFO("Disconnected, reason %d.",
			p_ble_evt->evt.gap_evt.params.disconnected.reason);
		m_conn_handle = BLE_CONN_HANDLE_INVALID;
		break;

#ifndef S140
	case BLE_GAP_EVT_PHY_UPDATE_REQUEST:
	{
		NRF_LOG_DEBUG("PHY update request.");
		ble_gap_phys_t const phys =
		{
			.rx_phys = BLE_GAP_PHY_AUTO,
			.tx_phys = BLE_GAP_PHY_AUTO,
		};
		err_code = sd_ble_gap_phy_update(p_ble_evt->evt.gap_evt.conn_handle, &phys);
		APP_ERROR_CHECK(err_code);
	} break;
#endif

	case BLE_GATTC_EVT_TIMEOUT:
		// Disconnect on GATT Client timeout event.
		NRF_LOG_DEBUG("GATT Client Timeout.");
		err_code = sd_ble_gap_disconnect(p_ble_evt->evt.gattc_evt.conn_handle,
			BLE_HCI_REMOTE_USER_TERMINATED_CONNECTION);
		APP_ERROR_CHECK(err_code);
		break;

	case BLE_GATTS_EVT_TIMEOUT:
		// Disconnect on GATT Server timeout event.
		NRF_LOG_DEBUG("GATT Server Timeout.");
		err_code = sd_ble_gap_disconnect(p_ble_evt->evt.gatts_evt.conn_handle,
			BLE_HCI_REMOTE_USER_TERMINATED_CONNECTION);
		APP_ERROR_CHECK(err_code);
		break;

	case BLE_EVT_USER_MEM_REQUEST:
		err_code = sd_ble_user_mem_reply(m_conn_handle, NULL);
		APP_ERROR_CHECK(err_code);
		break;

	case BLE_GATTS_EVT_RW_AUTHORIZE_REQUEST:
	{
		ble_gatts_evt_rw_authorize_request_t  req;
		ble_gatts_rw_authorize_reply_params_t auth_reply;

		req = p_ble_evt->evt.gatts_evt.params.authorize_request;

		if (req.type != BLE_GATTS_AUTHORIZE_TYPE_INVALID)
		{
			if ((req.request.write.op == BLE_GATTS_OP_PREP_WRITE_REQ) ||
				(req.request.write.op == BLE_GATTS_OP_EXEC_WRITE_REQ_NOW) ||
				(req.request.write.op == BLE_GATTS_OP_EXEC_WRITE_REQ_CANCEL))
			{
				if (req.type == BLE_GATTS_AUTHORIZE_TYPE_WRITE)
				{
					auth_reply.type = BLE_GATTS_AUTHORIZE_TYPE_WRITE;
				}
				else
				{
					auth_reply.type = BLE_GATTS_AUTHORIZE_TYPE_READ;
				}
				auth_reply.params.write.gatt_status = APP_FEATURE_NOT_SUPPORTED;
				err_code = sd_ble_gatts_rw_authorize_reply(p_ble_evt->evt.gatts_evt.conn_handle,
					&auth_reply);
				APP_ERROR_CHECK(err_code);
			}
		}
	} break;


	default:
		// No implementation needed.
		break;
	}
}



/**@brief Function for initializing the BLE stack.
 *
 * @details Initializes the SoftDevice and the BLE event interrupt.
 */
static void ble_stack_init(void)
{
	ret_code_t err_code;

	err_code = nrf_sdh_enable_request();
	APP_ERROR_CHECK(err_code);

	// Configure the BLE stack using the default settings.
	// Fetch the start address of the application RAM.
	uint32_t ram_start = 0;
	err_code = nrf_sdh_ble_default_cfg_set(APP_BLE_CONN_CFG_TAG, &ram_start);
	APP_ERROR_CHECK(err_code);

	// Enable BLE stack.
	err_code = nrf_sdh_ble_enable(&ram_start);
	APP_ERROR_CHECK(err_code);

	// Register a handler for BLE events.
	NRF_SDH_BLE_OBSERVER(m_ble_observer, APP_BLE_OBSERVER_PRIO, ble_evt_handler, NULL);
}

/**@brief Function for the Peer Manager initialization.
 */
static void peer_manager_init(void)
{
	ble_gap_sec_params_t sec_param;
	ret_code_t           err_code;

	err_code = pm_init();
	APP_ERROR_CHECK(err_code);

	memset(&sec_param, 0, sizeof(ble_gap_sec_params_t));

	// Security parameters to be used for all security procedures.
	sec_param.bond = SEC_PARAM_BOND;
	sec_param.mitm = SEC_PARAM_MITM;
	sec_param.io_caps = SEC_PARAM_IO_CAPABILITIES;
	sec_param.oob = SEC_PARAM_OOB;
	sec_param.min_key_size = SEC_PARAM_MIN_KEY_SIZE;
	sec_param.max_key_size = SEC_PARAM_MAX_KEY_SIZE;
	sec_param.kdist_own.enc = 1;
	sec_param.kdist_own.id = 1;
	sec_param.kdist_peer.enc = 1;
	sec_param.kdist_peer.id = 1;

	err_code = pm_sec_params_set(&sec_param);
	APP_ERROR_CHECK(err_code);

	err_code = pm_register(pm_evt_handler);
	APP_ERROR_CHECK(err_code);

	err_code = fds_register(fds_evt_handler);
	APP_ERROR_CHECK(err_code);
}


/**@brief Function for initializing the Advertising functionality.
 */
static void advertising_init(void)
{
	ret_code_t             err_code;
	ble_advertising_init_t init;

	memset(&init, 0, sizeof(init));

	create_adv_data_packet(&init.advdata);
	init.config.ble_adv_fast_enabled = true;
	init.config.ble_adv_fast_interval = APP_ADV_INTERVAL;
	init.config.ble_adv_fast_timeout = APP_ADV_TIMEOUT_IN_SECONDS;
	init.config.ble_adv_slow_enabled = true;
	init.config.ble_adv_slow_interval = 12800; // 8 Sekunden
	init.config.ble_adv_slow_timeout = 0;

	init.evt_handler = on_adv_evt;

	err_code = ble_advertising_init(&m_advertising, &init);

	APP_ERROR_CHECK(err_code);

	ble_advertising_conn_cfg_tag_set(&m_advertising, APP_BLE_CONN_CFG_TAG);
}


/**@brief Function for initializing the nrf log module.
 */
static void log_init(void)
{
	ret_code_t err_code = NRF_LOG_INIT(NULL);
	APP_ERROR_CHECK(err_code);

	NRF_LOG_DEFAULT_BACKENDS_INIT();
}


static void battery_management_callback(battery_state_t *battery_state) {
	ret_code_t err_code;
	uint8_t  battery_level;

	battery_level = (uint8_t)(100 * ((float)(battery_state->value) / (float)1023));
	err_code = ble_bas_battery_level_update(&m_bas, battery_level);
	if ((err_code != NRF_SUCCESS) &&
		(err_code != NRF_ERROR_INVALID_STATE) &&
		(err_code != NRF_ERROR_RESOURCES) &&
		(err_code != BLE_ERROR_GATTS_SYS_ATTR_MISSING)
		)
	{
		APP_ERROR_HANDLER(err_code);
	}
	if (battery_state->battery_low_warning) {
		power_off();
	}
}

static void power_management_init(void)
{
	uint32_t err_code = nrf_pwr_mgmt_init();
	APP_ERROR_CHECK(err_code);
}


/**@brief Function for application main entry.
 */
int main(void)
{
	log_init();
	timers_init();
	power_management_init();
	ble_stack_init();
	gap_params_init();
	gatt_init();
	services_init();
	advertising_init();
	conn_params_init();
	peer_manager_init();

	memset(&time_state.cts_date, 0, sizeof(cts_date_t));
	time_state.time_known = false;

	NRF_LOG_INFO("Digit app started.");
	display_init();
	application_timers_start();

	advertising_start(false);

	battery_management_init(battery_management_callback);
	battery_management_trigger();
	sd_power_dcdc_mode_set(NRF_POWER_DCDC_ENABLE);

	//power_off();

	for (;;)
	{
		if (NRF_LOG_PROCESS() == false)
		{
			nrf_pwr_mgmt_run();
		}
	}
}


