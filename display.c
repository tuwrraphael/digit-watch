#include "nrf_drv_spi.h"
#include "nrf_gpio.h"
#include "app_error.h"
#include <string.h>
#include "nrf_log.h"
#include "nrf_log_ctrl.h"
#include "nrf_log_default_backends.h"
#include "nrf_delay.h"
#include "nrf_drv_timer.h"
#include "nrf_drv_ppi.h"
#include "nrf_drv_gpiote.h"
#include "display.h"

#define DISPLAY_SCS (6)
#define DISPLAY_EXTCOMIN (8)
#define DISPLAY_DISP (11)

#define SPI_INSTANCE  1 /**< SPI instance index. */
static const nrf_drv_spi_t spi = NRF_DRV_SPI_INSTANCE(SPI_INSTANCE);  /**< SPI instance. */
static volatile bool spi_xfer_done;  /**< Flag used to indicate that SPI instance completed the transfer. */

									 /**
									 * @brief SPI user event handler.
									 * @param event
									 */
void spi_event_handler(nrf_drv_spi_evt_t const * p_event,
	void *                    p_context)
{
	spi_xfer_done = true;
}

static nrf_drv_timer_t timer = NRF_DRV_TIMER_INSTANCE(1); //TIMER 0 is reserved by softdevice

void timer_dummy_handler(nrf_timer_event_t event_type, void * p_context) {}

static void extcomin_setup()
{
	uint32_t compare_evt_addr;
	uint32_t gpiote_task_addr;
	nrf_ppi_channel_t ppi_channel;
	ret_code_t err_code;
	nrf_drv_gpiote_out_config_t config = GPIOTE_CONFIG_OUT_TASK_TOGGLE(false);

	err_code = nrf_drv_gpiote_out_init(DISPLAY_EXTCOMIN, &config);
	APP_ERROR_CHECK(err_code);


	nrf_drv_timer_extended_compare(&timer, (nrf_timer_cc_channel_t)0, 1000 /* ms */ * 1000UL, NRF_TIMER_SHORT_COMPARE0_CLEAR_MASK, false);

	err_code = nrf_drv_ppi_channel_alloc(&ppi_channel);
	APP_ERROR_CHECK(err_code);

	compare_evt_addr = nrf_drv_timer_event_address_get(&timer, NRF_TIMER_EVENT_COMPARE0);
	gpiote_task_addr = nrf_drv_gpiote_out_task_addr_get(DISPLAY_EXTCOMIN);

	err_code = nrf_drv_ppi_channel_assign(ppi_channel, compare_evt_addr, gpiote_task_addr);
	APP_ERROR_CHECK(err_code);

	err_code = nrf_drv_ppi_channel_enable(ppi_channel);
	APP_ERROR_CHECK(err_code);

	nrf_drv_gpiote_out_task_enable(DISPLAY_EXTCOMIN);
}

void init_display() {
	nrf_gpio_cfg_output(DISPLAY_SCS); //SCS
	nrf_gpio_cfg_output(DISPLAY_DISP); //DSP
	nrf_gpio_cfg_output(DISPLAY_EXTCOMIN); //DSP

	nrf_gpio_pin_write(DISPLAY_SCS, 0);
	nrf_delay_ms(1);
	nrf_gpio_pin_write(DISPLAY_DISP, 0);

	ret_code_t err_code;

	err_code = nrf_drv_ppi_init();
	APP_ERROR_CHECK(err_code);

	//err_code = nrf_drv_gpiote_init();// this is already initialized by the app_button library
	//APP_ERROR_CHECK(err_code);

	nrf_drv_timer_config_t timer_cfg = NRF_DRV_TIMER_DEFAULT_CONFIG;
	err_code = nrf_drv_timer_init(&timer, &timer_cfg, timer_dummy_handler);
	APP_ERROR_CHECK(err_code);

	extcomin_setup();

	nrf_drv_timer_enable(&timer);

	nrf_drv_spi_config_t spi_config = NRF_DRV_SPI_DEFAULT_CONFIG;
	spi_config.ss_pin = NRF_DRV_SPI_PIN_NOT_USED;
	spi_config.miso_pin = NRF_DRV_SPI_PIN_NOT_USED;
	spi_config.mosi_pin = 4;
	spi_config.sck_pin = 3;
	spi_config.frequency = NRF_DRV_SPI_FREQ_1M;
	spi_config.mode = NRF_DRV_SPI_MODE_0;
	spi_config.bit_order = NRF_DRV_SPI_BIT_ORDER_LSB_FIRST;
	APP_ERROR_CHECK(nrf_drv_spi_init(&spi, &spi_config, NULL, NULL));


	nrf_delay_ms(200);

	nrf_gpio_pin_write(DISPLAY_DISP, 1);

	uint8_t m_tx_buf[] = { 4,0 };

	nrf_gpio_pin_write(DISPLAY_SCS, 1);
	nrf_delay_ms(1);
	APP_ERROR_CHECK(nrf_drv_spi_transfer(&spi, m_tx_buf, sizeof(m_tx_buf), NULL, 0));
	nrf_gpio_pin_write(DISPLAY_SCS, 0);
	nrf_delay_ms(1);

	nrf_delay_ms(10);



	uint8_t m_tx_buf2[] = { 1,64,
		255,0,255,0,255,0,255,0,255,0,255,0,255,0,255,0,
		0,65,
		255,0,255,0,255,0,255,0,255,0,255,0,255,0,255,0,
		0,66,
		255,0,255,0,255,0,255,0,255,0,255,0,255,0,255,0,
		0,67,
		255,0,255,0,255,0,255,0,255,0,255,0,255,0,255,0,
		0,68,
		255,0,255,0,255,0,255,0,255,0,255,0,255,0,255,0,
		0,69,
		255,0,255,0,255,0,255,0,255,0,255,0,255,0,255,0,
		0,70,
		255,0,255,0,255,0,255,0,255,0,255,0,255,0,255,0,
		0,0 };

	nrf_gpio_pin_write(DISPLAY_SCS, 1);
	nrf_delay_ms(1);

	APP_ERROR_CHECK(nrf_drv_spi_transfer(&spi, m_tx_buf2, sizeof(m_tx_buf2), NULL, 0));

	nrf_gpio_pin_write(DISPLAY_SCS, 0);
	nrf_delay_ms(1);

	nrf_delay_ms(10);

	uint8_t m_tx_buf3[] = { 0,0 };

	nrf_gpio_pin_write(DISPLAY_SCS, 1);
	nrf_delay_ms(1);

	APP_ERROR_CHECK(nrf_drv_spi_transfer(&spi, m_tx_buf3, sizeof(m_tx_buf3), NULL, 0));

	nrf_gpio_pin_write(DISPLAY_SCS, 0);
	nrf_delay_ms(1);

	nrf_delay_ms(10);
}