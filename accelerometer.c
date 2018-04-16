
#include "sdk_errors.h"
#include "nrf_twi.h"
#include "nrf_drv_twi.h"
#include "app_util_platform.h"
#include "nrf_gpio.h"
#include "nrf_drv_gpiote.h"
#include "nrf_log.h"
#include "accelerometer.h"

#define TWI_INSTANCE_ID 1
static const nrf_drv_twi_t m_twi = NRF_DRV_TWI_INSTANCE(TWI_INSTANCE_ID);

#define LIS2DH_ADDR (0x19)

#define LIS2DH_CTRL_REG1 (0x20)
#define CTRL_REG1_ODR_1HZ (1 << 4)
#define CTRL_REG1_LP_EN (1 << 3)
#define CTRL_REG1_Z_EN (1 << 2)
#define CTRL_REG1_Y_EN (1 << 1)
#define CTRL_REG1_X_EN (1 << 0)
#define LIS2DH_CTRL_REG3 (0x22)
#define CTRL_REG3_CLICK_PIN1_EN (1 << 7)
#define CTRL_REG3_AOI1_PIN1_EN (1 << 6)
#define LIS2DH_INT1_CFG (0x30)
#define INT1_CFG_6D_EN (1 << 6)
#define LIS2DH_INT1_THS (0x32)
#define LIS2DH_INT1_DURATION (0x33)

#define REG1_NORMAL_MODE (CTRL_REG1_ODR_1HZ | CTRL_REG1_LP_EN | CTRL_REG1_Z_EN \
	| CTRL_REG1_Y_EN | CTRL_REG1_X_EN)

#define REG3_INT_CONFIG (CTRL_REG3_AOI1_PIN1_EN | CTRL_REG3_CLICK_PIN1_EN)

typedef enum {
	INIT_STATUS_NOT_INITIALIZED,
	INIT_STATUS_CONFIGURE_REG1,
	INIT_STATUS_CONFIGURE_REG3,
	INIT_STATUS_CONFIGURE_INT1,
	INIT_STATUS_CONFIGURE_INT1_THS,
	INIT_STATUS_CONFIGURE_INT1_DUR,
	INIT_STATUS_INITIALIZED
} acc_init_status_t;

static acc_init_status_t acc_init_status = INIT_STATUS_NOT_INITIALIZED;

static ret_code_t lis2dh_write_reg(uint8_t reg, uint8_t value) {
	uint8_t data[2] = { reg, value };
	return nrf_drv_twi_tx(&m_twi, LIS2DH_ADDR, data, sizeof(data), false);
}

static ret_code_t accelerometer_init_task() {
	switch (acc_init_status)
	{
	case INIT_STATUS_NOT_INITIALIZED:
		nrf_drv_twi_enable(&m_twi);
		acc_init_status = INIT_STATUS_CONFIGURE_REG1;
		return lis2dh_write_reg(LIS2DH_CTRL_REG1, REG1_NORMAL_MODE);
	case INIT_STATUS_CONFIGURE_REG1:
		acc_init_status = INIT_STATUS_CONFIGURE_REG3;
		return lis2dh_write_reg(LIS2DH_CTRL_REG3, REG3_INT_CONFIG);
	case INIT_STATUS_CONFIGURE_REG3:
		acc_init_status = INIT_STATUS_CONFIGURE_INT1;
		return lis2dh_write_reg(LIS2DH_INT1_CFG, INT1_CFG_6D_EN);
	case INIT_STATUS_CONFIGURE_INT1:
		acc_init_status = INIT_STATUS_CONFIGURE_INT1_THS;
		return lis2dh_write_reg(LIS2DH_INT1_THS, 1);
	case INIT_STATUS_CONFIGURE_INT1_THS:
		acc_init_status = INIT_STATUS_CONFIGURE_INT1_DUR;
		return lis2dh_write_reg(LIS2DH_INT1_DURATION, 1);
	case INIT_STATUS_CONFIGURE_INT1_DUR:
		acc_init_status = INIT_STATUS_INITIALIZED;
		nrf_drv_twi_disable(&m_twi);
		return NRF_SUCCESS;
	default:
		return NRF_ERROR_INVALID_STATE;
	}
}

static void twi_handler(nrf_drv_twi_evt_t const * p_event, void * p_context)
{
	NRF_LOG_INFO("twi %d", p_event->type);
	switch (p_event->type)
	{
	case NRF_DRV_TWI_EVT_DONE:
		NRF_LOG_INFO("xfer %d", p_event->xfer_desc.type);
		if (p_event->xfer_desc.type == NRF_DRV_TWI_XFER_TX)
		{
			if (acc_init_status != INIT_STATUS_NOT_INITIALIZED &&
				acc_init_status != INIT_STATUS_INITIALIZED) {

				accelerometer_init_task();
			}
		}
		break;
	default:
		break;
	}
}

static ret_code_t twi_init(void)
{
	const nrf_drv_twi_config_t twi_lis2dh_config = {
		.scl = ACC_SCL,
		.sda = ACC_SDA,
		.frequency = NRF_TWI_FREQ_100K,
		.interrupt_priority = APP_IRQ_PRIORITY_HIGH,
		.clear_bus_init = false
	};
	return nrf_drv_twi_init(&m_twi, &twi_lis2dh_config, twi_handler, NULL);
}


static void int_event_handler(nrf_drv_gpiote_pin_t pin, nrf_gpiote_polarity_t action) {
	NRF_LOG_INFO("pin interrupt");
}


ret_code_t accelerometer_init() {
	if (!nrf_drv_gpiote_is_init())
	{
		VERIFY_SUCCESS(nrf_drv_gpiote_init());
	}
	//nrf_gpio_cfg_sense_input(ACC_INT1, NRF_GPIO_PIN_NOPULL, NRF_GPIO_PIN_SENSE_HIGH); //wakeup interrupt
	nrf_drv_gpiote_in_config_t config = GPIOTE_CONFIG_IN_SENSE_LOTOHI(false);
	nrf_drv_gpiote_in_init(ACC_INT1, &config, int_event_handler);
	nrf_drv_gpiote_in_event_enable(ACC_INT1, true);
	VERIFY_SUCCESS(twi_init());
	//VERIFY_SUCCESS(accelerometer_init_task());
	nrf_drv_twi_enable(&m_twi);
	ret_code_t err_code = lis2dh_write_reg(LIS2DH_CTRL_REG1, REG1_NORMAL_MODE);
	NRF_LOG_INFO("acc initialized, %d", err_code);
	return NRF_SUCCESS;
}