#include "nrf_drv_spi.h"
#include "nrf_gpio.h"
#include "app_error.h"
#include <string.h>
#include "nrf_delay.h"
#include "nrf_gfx.h"
#include "nrf_lcd.h"
#include <math.h>
#include "nrf_log.h"

#include "./display_extcomin.h"
#include "./display.h"

#define SPI_INSTANCE 1 //SPI 0 is blocked by softdevice
#define M_PI		(3.14159265358979323846)

static const nrf_drv_spi_t spi = NRF_DRV_SPI_INSTANCE(SPI_INSTANCE);
static uint32_t display_buffer[4 * 128];
static bool display_enabled = false;

__STATIC_INLINE void scs_set()
{
	nrf_gpio_pin_write(DISPLAY_SCS, 1);
	nrf_delay_us(4);
}

__STATIC_INLINE void scs_release()
{
	nrf_delay_us(1);
	nrf_gpio_pin_write(DISPLAY_SCS, 0);
	nrf_delay_us(1);
}

static void gpio_setup() {
	nrf_gpio_cfg_output(DISPLAY_SCS);
	nrf_gpio_cfg_output(DISPLAY_DISP);

	nrf_gpio_pin_write(DISPLAY_SCS, 0);
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


static void clear_display_buffer() {
	memset(display_buffer, 0xFFFFFFFF, sizeof(display_buffer));
}

static ret_code_t disp_def_init(void)
{
	clear_display_buffer();
	return NRF_SUCCESS;
}

static void disp_def_uninit(void)
{

}

static void disp_def_pixel_draw(uint16_t m_x, uint16_t m_y, uint32_t color)
{
	uint16_t x = 128 - m_y;
	uint16_t y = m_x;
	uint16_t index = 4 * y + (x / 32);
	uint32_t mask = (1 << x % 32);
	if (color) {
		display_buffer[index] &= ~(mask);
	}
	else {
		display_buffer[index] |= mask;
	}
}

static void disp_def_rect_draw(uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint32_t color)
{
	uint16_t i, z;
	for (i = 0; i < height; i++) {
		for (z = 0; z < width; z++) {
			disp_def_pixel_draw(x + z, y + i, color);
		}
	}
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

static void format_line_from_buffer(uint8_t *buf, uint8_t linenr) {
	*buf = 0;
	buf++;
	*buf = linenr;
	uint16_t buffer_offset = linenr * 4;
	uint8_t shift;
	uint8_t index;
	for (index = 0; index < 4; index++) {
		for (shift = 0; shift < 32; shift += 8) {
			buf++;
			*buf = (display_buffer[buffer_offset + index] >> shift) & 0xFF;
		}
	}
}

//extern const nrf_gfx_font_desc_t orkney_8ptFontInfo;
//static const nrf_gfx_font_desc_t * p_font = &orkney_8ptFontInfo;

static void init_display_spi() {
	uint8_t m_tx_buf[] = { 4,0 };
	scs_set();
	APP_ERROR_CHECK(nrf_drv_spi_transfer(&spi, m_tx_buf, sizeof(m_tx_buf), NULL, 0));
	scs_release();
}

void transfer_buffer_to_display() {
	uint8_t m_tx_buf2[254];
	uint8_t z;
	uint8_t i;
	if (!display_enabled) {
		return;
	}
	spi_setup();
	for (z = 0; z < 10; z++)
	{
		for (i = 0; i < 14 && ((z * 14) + i) < 129; i++) {
			format_line_from_buffer(&m_tx_buf2[i * 18], 1 + (z * 14) + i);
		}

		m_tx_buf2[0] = 1;

		m_tx_buf2[14 * 18] = 0;
		m_tx_buf2[14 * 18 + 1] = 0;

		scs_set();
		APP_ERROR_CHECK(nrf_drv_spi_transfer(&spi, m_tx_buf2, 14 * 18 + 2, NULL, 0));
		scs_release();
	}
	clear_display_buffer();
}

void switch_display_mode() {
	if (!display_enabled) {
		return;
	}
	uint8_t m_tx_buf3[] = { 0,0 };
	scs_set();
	APP_ERROR_CHECK(nrf_drv_spi_transfer(&spi, m_tx_buf3, sizeof(m_tx_buf3), NULL, 0));
	scs_release();
	nrf_drv_spi_uninit(&spi);
}

void draw_time_indicator(float s, float indicator_length, uint8_t thickness) {

	float arg = ((float)(15 - s)*M_PI) / ((float)30);
	uint8_t x = 64 + (cos(arg) * indicator_length);
	uint8_t y = 64 - (sin(arg) * indicator_length);
	if (x < 64 || y < 64) {
		nrf_gfx_line_t line = NRF_GFX_LINE(
			x,
			y,
			64,
			64, thickness);
		APP_ERROR_CHECK(nrf_gfx_line_draw(&display_definition, &line, 1));
	}
	else {
		nrf_gfx_line_t line = NRF_GFX_LINE(
			64,
			64,
			x,
			y, thickness);
		APP_ERROR_CHECK(nrf_gfx_line_draw(&display_definition, &line, 1));
	}
}

void display_init() {
	gpio_setup();
	extcomin_setup();
	display_enable();
	APP_ERROR_CHECK(nrf_gfx_init(&display_definition));
}

// ensure spi is down already
void display_disable() {
	display_enabled = false;
	nrf_gpio_pin_write(DISPLAY_DISP, 0);
	extcomin_disable();
	nrf_gpio_cfg_default(DISPLAY_SCK);
	nrf_gpio_cfg_default(DISPLAY_MOSI);
}

void display_enable() {
	extcomin_enable();
	spi_setup();
	nrf_gpio_pin_write(DISPLAY_DISP, 1);
	nrf_delay_us(100);
	init_display_spi();
	nrf_drv_spi_uninit(&spi);
	display_enabled = true;
}

void display_uninit() {
	display_disable();
	//TODO extcomin uninit
	nrf_gfx_uninit(&display_definition);
}