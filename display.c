#include "app_error.h"
#include <string.h>
#include "nrf_delay.h"
#include "nrf_gfx.h"
#include "nrf_lcd.h"
#include <math.h>
#include "nrf_log.h"
#include "nrfx_spim.h"
#include "nrfx_gpiote.h"

#include "./display_extcomin.h"
#include "./display.h"
#include "buffer_display.h"

#define SPI_INSTANCE 1 //SPI 0 is blocked by softdevice

static const nrfx_spim_t spi = NRFX_SPIM_INSTANCE(SPI_INSTANCE);
static bool display_enabled = false;
static bool display_initialized = false;
static bool disp_initialized = false;

static void gpio_setup()
{
	if (!nrfx_gpiote_is_init())
	{
		APP_ERROR_CHECK(nrfx_gpiote_init());
	}
	nrfx_gpiote_out_config_t config = NRFX_GPIOTE_CONFIG_OUT_SIMPLE(false);
	if (disp_initialized)
	{
		nrfx_gpiote_out_uninit(DISPLAY_DISP);
	}
	nrfx_gpiote_out_init(DISPLAY_DISP, &config);
	disp_initialized = true;
}

static void spi_setup()
{
	nrfx_spim_config_t spi_config = NRFX_SPIM_DEFAULT_CONFIG;
	spi_config.frequency = NRF_SPIM_FREQ_1M;
	spi_config.ss_pin = DISPLAY_SCS;
	spi_config.miso_pin = NRFX_SPIM_PIN_NOT_USED;
	spi_config.mosi_pin = DISPLAY_MOSI;
	spi_config.sck_pin = DISPLAY_SCK;
	spi_config.ss_active_high = true;
	spi_config.mode = NRF_SPIM_MODE_0;
	spi_config.bit_order = NRF_SPIM_BIT_ORDER_LSB_FIRST;
	APP_ERROR_CHECK(nrfx_spim_init(&spi, &spi_config, NULL, NULL));
}
static void format_line_from_buffer(uint8_t *buf, uint8_t linenr)
{
	*buf = 0;
	buf++;
	*buf = linenr;
	uint16_t buffer_offset = linenr * 4;
	uint8_t shift;
	uint8_t index;
	for (index = 0; index < 4; index++)
	{
		for (shift = 0; shift < 32; shift += 8)
		{
			buf++;
			*buf = (display_buffer[buffer_offset + index] >> shift) & 0xFF;
		}
	}
}

static void init_display_spi()
{
	uint8_t m_tx_buf[] = {4, 0};
	nrfx_spim_xfer_desc_t xfer_desc = NRFX_SPIM_XFER_TX(m_tx_buf, sizeof(m_tx_buf));
	APP_ERROR_CHECK(nrfx_spim_xfer(&spi, &xfer_desc, 0));
}

void transfer_buffer_to_display()
{
	uint8_t m_tx_buf2[254];
	uint8_t z;
	uint8_t i;
	if (!display_enabled)
	{
		return;
	}
	spi_setup();
	for (z = 0; z < 10; z++)
	{
		for (i = 0; i < 14 && ((z * 14) + i) < 129; i++)
		{
			format_line_from_buffer(&m_tx_buf2[i * 18], 1 + (z * 14) + i);
		}

		m_tx_buf2[0] = 1;

		m_tx_buf2[14 * 18] = 0;
		m_tx_buf2[14 * 18 + 1] = 0;
		uint8_t transmit_length = 14 * 18 + 2;
		nrfx_spim_xfer_desc_t xfer_desc = NRFX_SPIM_XFER_TX(m_tx_buf2, transmit_length);
		APP_ERROR_CHECK(nrfx_spim_xfer(&spi, &xfer_desc, 0));
	}
}

void switch_display_mode()
{
	if (!display_enabled)
	{
		return;
	}
	uint8_t m_tx_buf3[] = {0, 0};
	nrfx_spim_xfer_desc_t xfer_desc = NRFX_SPIM_XFER_TX(m_tx_buf3, sizeof(m_tx_buf3));
	APP_ERROR_CHECK(nrfx_spim_xfer(&spi, &xfer_desc, 0));
	nrfx_spim_uninit(&spi);
}

void display_init()
{
	if (!display_initialized)
	{
		display_initialized = true;
		gpio_setup();
		extcomin_setup();
		display_enable();
		APP_ERROR_CHECK(nrf_gfx_init(&nrf_lcd_buffer_display));
	}
}

// ensure spi is down already
void display_disable()
{
	display_enabled = false;
	nrfx_gpiote_out_clear(DISPLAY_DISP);
	extcomin_disable();
	nrfx_gpiote_out_uninit(DISPLAY_SCK);
	nrfx_gpiote_out_uninit(DISPLAY_MOSI);
}

void display_enable()
{
	extcomin_enable();
	spi_setup();
	nrfx_gpiote_out_set(DISPLAY_DISP);
	nrf_delay_us(100);
	init_display_spi();
	nrfx_spim_uninit(&spi);
	display_enabled = true;
}

void display_init_off() {
	gpio_setup();
	nrfx_gpiote_out_clear(DISPLAY_DISP);
}

void display_uninit()
{
	if (display_initialized)
	{
		display_initialized = false;
		display_disable();
		extcomin_uninit();		
		nrf_gfx_uninit(&nrf_lcd_buffer_display);
	}
}