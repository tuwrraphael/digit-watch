
#include "sdk_errors.h"
#include "nrf_twi.h"
#include "nrf_drv_twi.h"
#include "app_util_platform.h"

#define ACC_SCL (18)
#define ACC_SDA (16)
#define ACC_INT1 (14)

#define TWI_INSTANCE_ID 0
static const nrf_drv_twi_t m_twi = NRF_DRV_TWI_INSTANCE(TWI_INSTANCE_ID);

static void twi_init(void)
{
	ret_code_t err_code;

	const nrf_drv_twi_config_t twi_lis2dh_config = {
		.scl = ACC_SCL,
		.sda = ACC_SDA,
		.frequency = NRF_TWI_FREQ_100K,
		.interrupt_priority = APP_IRQ_PRIORITY_HIGH,
		.clear_bus_init = false
	};

	err_code = nrf_drv_twi_init(&m_twi, &twi_lis2dh_config, twi_handler, NULL);
	APP_ERROR_CHECK(err_code);

	nrf_drv_twi_enable(&m_twi);
}

void twi_handler(nrf_drv_twi_evt_t const * p_event, void * p_context)
{
	switch (p_event->type)
	{
	case NRF_DRV_TWI_EVT_DONE:
		if (p_event->xfer_desc.type == NRF_DRV_TWI_XFER_RX)
		{
			data_handler(m_sample);
		}
		m_xfer_done = true;
		break;
	default:
		break;
	}
}

#define LIS2DH_CTRL_REG1 (0x20)

static void set_mode(void)
{
	ret_code_t err_code;

	/* Writing to LM75B_REG_CONF "0" set temperature sensor in NORMAL mode. */
	uint8_t reg[2] = { LIS2DH_CTRL_REG1, NORMAL_MODE };
	err_code = nrf_drv_twi_tx(&m_twi, LM75B_ADDR, reg, sizeof(reg), false);
	APP_ERROR_CHECK(err_code);
	while (m_xfer_done == false);

	/* Writing to pointer byte. */
	reg[0] = LM75B_REG_TEMP;
	m_xfer_done = false;
	err_code = nrf_drv_twi_tx(&m_twi, LM75B_ADDR, reg, 1, false);
	APP_ERROR_CHECK(err_code);
	while (m_xfer_done == false);
}

void accelerometer_init() {
	twi_init();
}