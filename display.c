#include "nrf_drv_spi.h"
#include "nrf_gpio.h"
#include "app_error.h"
#include <string.h>
#include "nrf_delay.h"
#include "nrf_gfx.h"
#include "nrf_lcd.h"

#include "./display_extcomin.h"
#include "./display.h"

#define SPI_INSTANCE 1 //SPI 0 is blocked by softdevice

static const nrf_drv_spi_t spi = NRF_DRV_SPI_INSTANCE(SPI_INSTANCE);

static void gpio_setup() {
	nrf_gpio_cfg_output(DISPLAY_SCS);
	nrf_gpio_cfg_output(DISPLAY_DISP);

	nrf_gpio_pin_write(DISPLAY_SCS, 0);
	nrf_delay_ms(1);
	nrf_gpio_pin_write(DISPLAY_DISP, 0);
}

static void spi_setup() {
	nrf_drv_spi_config_t spi_config = NRF_DRV_SPI_DEFAULT_CONFIG;
	spi_config.ss_pin = NRF_DRV_SPI_PIN_NOT_USED;
	spi_config.miso_pin = NRF_DRV_SPI_PIN_NOT_USED;
	spi_config.mosi_pin = DISPLAY_MOSI;
	spi_config.sck_pin = DISPLAY_SCK;
	spi_config.frequency = NRF_DRV_SPI_FREQ_1M;
	spi_config.mode = NRF_DRV_SPI_MODE_0;
	spi_config.bit_order = NRF_DRV_SPI_BIT_ORDER_LSB_FIRST;
	APP_ERROR_CHECK(nrf_drv_spi_init(&spi, &spi_config, NULL, NULL));
}

static ret_code_t disp_def_init(void)
{
	return NRF_SUCCESS;
}

static void disp_def_uninit(void)
{
	//nrf_drv_spi_uninit(&spi);
}

static void disp_def_pixel_draw(uint16_t x, uint16_t y, uint32_t color)
{
}

static void disp_def_rect_draw(uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint32_t color)
{
}

static void disp_def_dummy_display(void)
{
}

static void disp_def_rotation_set(nrf_lcd_rotation_t rotation)
{
}

static void disp_def_display_invert(bool invert)
{
}

static lcd_cb_t display_cb = {
	.height = DISPLAY_HEIGHT,
	.width = DISPLAY_WIDTH
};

const nrf_lcd_t display_definition = {
	.lcd_init = disp_def_init,
	.lcd_uninit = disp_def_uninit,
	.lcd_pixel_draw = disp_def_pixel_draw,
	.lcd_rect_draw = disp_def_rect_draw,
	.lcd_display = disp_def_dummy_display,
	.lcd_rotation_set = disp_def_rotation_set,
	.lcd_display_invert = disp_def_display_invert,
	.p_lcd_cb = &display_cb
};

static void gfx_setup() {
	
}

void init_display() {
	gpio_setup();
	extcomin_setup();
	spi_setup();

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