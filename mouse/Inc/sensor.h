/* MIT License
 *
 * Copyright (c) 2023 Zaunkoenig GmbH
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#pragma once

#include <stdint.h>
#include "delay.h"
#include "board.h"
#include "stm32f7xx.h"
#include "config.h"


#ifdef BOARD_M2K
	#include "srom_3360_0x05.h"
	#define DPI_MIN        0x0000 // =   0 =   100 dpi
	#define DPI_MAX        0x0077 // = 119 = 20000 dpi
	#define DPI_LARGE_JUMP 5      // 5 * 100 = 500 dpi

	static void spi_init(void)
	{
		// GPIO config
		SPIx_SCK_GPIO_CLK_ENABLE();
		SPIx_MISO_GPIO_CLK_ENABLE();
		SPIx_MOSI_GPIO_CLK_ENABLE();
		SPIx_SS_GPIO_CLK_ENABLE();

		// PA5 SCK
		MODIFY_REG(SPIx_SCK_GPIO_PORT->MODER,
				0b11 << (2*SPIx_SCK_PIN_Pos),
				0b10 << (2*SPIx_SCK_PIN_Pos));
		MODIFY_REG(SPIx_SCK_GPIO_PORT->AFR[SPIx_SCK_PIN_Pos >= 8],
				0b1111 << ((4*SPIx_SCK_PIN_Pos)%32),
				SPIx_SCK_AF << ((4*SPIx_SCK_PIN_Pos)%32));
		// PA6 MISO
		MODIFY_REG(SPIx_MISO_GPIO_PORT->MODER,
				0b11 << (2*SPIx_MISO_PIN_Pos),
				0b10 << (2*SPIx_MISO_PIN_Pos));
		MODIFY_REG(SPIx_MISO_GPIO_PORT->PUPDR,
				0b11 << (2*SPIx_MISO_PIN_Pos),
				0b10 << (2*SPIx_MISO_PIN_Pos));
		MODIFY_REG(SPIx_MISO_GPIO_PORT->AFR[SPIx_MISO_PIN_Pos >= 8],
				0b1111 << ((4*SPIx_MISO_PIN_Pos)%32),
				SPIx_MISO_AF << ((4*SPIx_MISO_PIN_Pos)%32));
		// PA7 MOSI
		MODIFY_REG(SPIx_MOSI_GPIO_PORT->MODER,
				0b11 << (2*SPIx_MOSI_PIN_Pos),
				0b10 << (2*SPIx_MOSI_PIN_Pos));
		MODIFY_REG(SPIx_MOSI_GPIO_PORT->AFR[SPIx_MOSI_PIN_Pos >= 8],
				0b1111 << ((4*SPIx_MOSI_PIN_Pos)%32),
				SPIx_MOSI_AF << ((4*SPIx_MOSI_PIN_Pos)%32));
		// PB6 SS
		MODIFY_REG(SPIx_SS_PORT->MODER,
				0b11 << (2*SPIx_SS_PIN_Pos),
				0b01 << (2*SPIx_SS_PIN_Pos));

		// SPI config
		SPIx_CLK_ENABLE();
		SPIx->CR1 = SPI_CR1_SSM | SPI_CR1_SSI // software SS
				| (0b011 << SPI_CR1_BR_Pos) // assumes PCLK2 = 32MHz. divide by 16 for 2MHz
				| SPI_CR1_MSTR // master
				| SPI_CR1_CPOL // CPOL = 1
				| SPI_CR1_CPHA; // CPHA = 1
		SPIx->CR2 = SPI_CR2_FRXTH // 8-bit level for RXNE
				| (0b0111 << SPI_CR2_DS_Pos); // 8-bit data
		SPIx->CR1 |=  SPI_CR1_SPE; // enable SPI
	}

	static inline void ss_low(void)
	{
		SPIx_SS_PORT->ODR &= ~SPIx_SS_PIN;
	}

	static inline void ss_high(void)
	{
		SPIx_SS_PORT->ODR |= SPIx_SS_PIN;
	}

	static inline uint8_t spi_sendrecv(uint8_t b)
	{
		while (!(SPIx->SR & SPI_SR_TXE));
		*(__IO uint8_t *)&SPIx->DR = b;
		while (!(SPIx->SR & SPI_SR_RXNE));
		return *(__IO uint8_t *)&SPIx->DR;
	}

#define spi_recv(x) spi_sendrecv(0)
#define spi_send(x) (void)spi_sendrecv(x)

	static void spi_write(const uint8_t addr, const uint8_t data) {
		spi_send(addr | 0x80);
		spi_send(data);
		delay_us(180); // maximum of t_SWW, t_SWR
	}

	static uint8_t spi_read(const uint8_t addr) {
		spi_send(addr);
		delay_us(160); // t_SRAD
		uint8_t rd = spi_recv();
		delay_us(20);
		return rd;
	}

	static void sensor_init(const Config cfg)
	{
		const uint16_t dpi = _FLD2VAL(CONFIG_DPI, cfg);
		const uint8_t ang_snap = (cfg & CONFIG_ANGLE_SNAP_ON) ? 1 : 0;
		const uint8_t lod = _FLD2VAL(CONFIG_LOD, cfg);

		ss_high();
		delay_ms(3);

		// shutdown first
		ss_low();
		spi_write(0x3b, 0xb6);
		ss_high();
		delay_ms(300);

		// drop and raise ncs to reset spi port
		ss_low();
		delay_us(40);
		ss_high();
		delay_us(40);

		// power up reset
		ss_low();
		spi_write(0x3a, 0x5a);
		ss_high();
		delay_ms(50);

		// read from 0x02 to 0x06
		ss_low();
		spi_read(0x02);
		spi_read(0x03);
		spi_read(0x04);
		spi_read(0x05);
		spi_read(0x06);

		spi_write(0x10, 0x00);
		spi_write(0x22, 0x00);

		// srom download
		spi_write(0x13, 0x1d);
		ss_high();
		delay_ms(10);
		ss_low();
		spi_write(0x13, 0x18);

		spi_send(0x62 | 0x80);
		for (uint16_t i = 0; i < SROM_LENGTH; i++) {
			delay_us(16);
			spi_send(srom[i]);
		}
		delay_us(18);
		ss_high();
		delay_us(200);

		// check srom id
		ss_low();

		// configuration/settings
		spi_write(0x10, 0x00); // 0x20 (g502 default) enables rest mode after ~10s of inactivity
		spi_write(0x14, 0xff); // how long to wait before going to rest mode. 0xff is max (~10 seconds)
		spi_write(0x17, 0xff);
		spi_write(0x18, 0x00);
		spi_write(0x19, 0x00);
		spi_write(0x1b, 0x00);
		spi_write(0x1c, 0x00);

		spi_write(0x2c, 0x0a);
		spi_write(0x2b, 0x10);

		// configuration/settings
		spi_write(0x0f, dpi);
		spi_write(0x42, (ang_snap << 7) | 0x0D); // angle snapping
		spi_write(0x63, lod); 	// lift off distance
		spi_write(0x0d, 0x60); 	// invert x,y
		ss_high();

		delay_us(100);

		// begin burst mode (write any value, i.e. 0x00 to burst mode register 0x50)
		ss_low();
		spi_write(0x50, 0x00);
		ss_high();
	}

	static void sensor_set_dpi(uint8_t dpi)
	{
		ss_low();
		spi_write(0x0f, dpi);
		spi_write(0x50, 0x00); // return to burst mode
		ss_high();
	}

	static void sensor_set_as(const uint8_t angle_snap)
	{
		ss_low();
		spi_write(0x42, (angle_snap << 7) | 0x0D);
		spi_write(0x50, 0x00); // return to burst mode
		ss_high();
	}

	static void sensor_set_lod(const uint8_t lod)
	{
		ss_low();
		spi_write(0x63, lod);
		spi_write(0x50, 0x00); // return to burst mode
		ss_high();
	}
#elif BOARD_M3K
	#define DPI_MIN        0x0000 // =   0 =    50 dpi
	#define DPI_MAX        0x018F // = 399 = 20000 dpi
	#define DPI_LARGE_JUMP 10     // 10 * 50 = 500 dpi

	// WLCSP100 Pin Mapping
	// SCK    GPIOA 5 PC10 A3 SPI3_SCK/I2S3_CK, USART3_TX, UART4_TX, QUADSPI_BK1_IO1, SDMMC1_D2, EVENTOUT
	// MISO   GPIOA 6 PB4  B7 NJTRST, TIM3_CH1, SPI1_MISO, SPI3_MISO, SPI2_NSS/I2S2_WS, SDMMC2_D3, EVENTOUT
	// MOSI   GPIOA 7 PB2  K6 SAI1_SD_A, SPI3_MOSI/I2S3_SD, QUADSPI_CLK, EVENTOUT
	// SS     GPIOB 6 PC9  D1 CO2, TIM3_CH4, TIM8_CH4, I2C3_SDA, I2S_CKIN, UART5_CTS, QUADSPI_BK1_IO0, SDMMC1_D1, EVENTOUT
	// NRESET GPIOA 8 PA8  D3 MCO1, TIM1_CH1, TIM8_BKIN2, I2C3_SCL, USART1_CK, OTG_FS_SOF, EVENTOUT

	static void spi_init(void)
	{
		// GPIO config
		SPIx_SCK_GPIO_CLK_ENABLE();
		SPIx_MISO_GPIO_CLK_ENABLE();
		SPIx_MOSI_GPIO_CLK_ENABLE();
		SPIx_SS_GPIO_CLK_ENABLE();

		// SCK
		MODIFY_REG(SPIx_SCK_GPIO_PORT->MODER,
				0b11 << (2*SPIx_SCK_PIN_Pos),
				0b10 << (2*SPIx_SCK_PIN_Pos));
		MODIFY_REG(SPIx_SCK_GPIO_PORT->AFR[SPIx_SCK_PIN_Pos >= 8],
				0b1111 << ((4*SPIx_SCK_PIN_Pos)%32),
				SPIx_SCK_AF << ((4*SPIx_SCK_PIN_Pos)%32));
		// MISO
		MODIFY_REG(SPIx_MISO_GPIO_PORT->MODER,
				0b11 << (2*SPIx_MISO_PIN_Pos),
				0b10 << (2*SPIx_MISO_PIN_Pos));
		MODIFY_REG(SPIx_MISO_GPIO_PORT->PUPDR,
				0b11 << (2*SPIx_MISO_PIN_Pos),
				0b10 << (2*SPIx_MISO_PIN_Pos));
		MODIFY_REG(SPIx_MISO_GPIO_PORT->AFR[SPIx_MISO_PIN_Pos >= 8],
				0b1111 << ((4*SPIx_MISO_PIN_Pos)%32),
				SPIx_MISO_AF << ((4*SPIx_MISO_PIN_Pos)%32));
		// MOSI
		MODIFY_REG(SPIx_MOSI_GPIO_PORT->MODER,
				0b11 << (2*SPIx_MOSI_PIN_Pos),
				0b10 << (2*SPIx_MOSI_PIN_Pos));
		MODIFY_REG(SPIx_MOSI_GPIO_PORT->AFR[SPIx_MOSI_PIN_Pos >= 8],
				0b1111 << ((4*SPIx_MOSI_PIN_Pos)%32),
				SPIx_MOSI_AF << ((4*SPIx_MOSI_PIN_Pos)%32));
		// SS
		MODIFY_REG(SPIx_SS_PORT->MODER,
				0b11 << (2*SPIx_SS_PIN_Pos),
				0b01 << (2*SPIx_SS_PIN_Pos));

		// SPI config
		SPIx_CLK_ENABLE();
		// CLOCK
		SPIx->CR1 = SPI_CR1_SSM | SPI_CR1_SSI // software SS
				| (0b001 << SPI_CR1_BR_Pos) // assumes PCLK2 = 32MHz. divide by 4 for 8MHz
				| SPI_CR1_MSTR // master
				| SPI_CR1_CPOL // CPOL = 1
				| SPI_CR1_CPHA; // CPHA = 1
		SPIx->CR2 = SPI_CR2_FRXTH // 8-bit level for RXNE
				| (0b0111 << SPI_CR2_DS_Pos); // 8-bit data
		SPIx->CR1 |=  SPI_CR1_SPE; // enable SPI
	}

	static inline void ss_low(void)
	{
		SPIx_SS_PORT->ODR &= ~SPIx_SS_PIN;
	}

	static inline void ss_high(void)
	{
		SPIx_SS_PORT->ODR |= SPIx_SS_PIN;
	}

	static inline uint8_t spi_sendrecv(uint8_t b)
	{
		while (!(SPIx->SR & SPI_SR_TXE));
		*(__IO uint8_t *)&SPIx->DR = b;
		while (!(SPIx->SR & SPI_SR_RXNE));
		return *(__IO uint8_t *)&SPIx->DR;
	}

#define spi_recv(x) spi_sendrecv(0)
#define spi_send(x) (void)spi_sendrecv(x)

	static void spi_write(const uint8_t addr, const uint8_t data) {
		spi_send(addr | 0x80);
		spi_send(data);
		delay_us(5); // maximum of t_SWW, t_SWR
	}

	static uint8_t spi_read(const uint8_t addr) {
		spi_send(addr);
		delay_us(2); // t_SRAD
		uint8_t rd = spi_recv();
		delay_us(2); // maximum of t_SRW, t_SRR
		return rd;
	}

	// equivalent of 6.2.1-99
	static void sensor_spi1(void)
	{
		ss_low();
		spi_write(0x40, 0x80);
		spi_write(0x7F, 0x0E);
		spi_write(0x55, 0x0D);
		spi_write(0x56, 0x1B);
		spi_write(0x57, 0xE8);
		spi_write(0x58, 0xD5);
		spi_write(0x7F, 0x14);
		spi_write(0x42, 0xBC);
		spi_write(0x43, 0x74);
		spi_write(0x4B, 0x20);
		spi_write(0x4D, 0x00);
		spi_write(0x53, 0x0D);
		spi_write(0x7F, 0x05);
		spi_write(0x51, 0x40);
		spi_write(0x53, 0x40);
		spi_write(0x55, 0xCA);
		spi_write(0x61, 0x31);
		spi_write(0x62, 0x64);
		spi_write(0x6D, 0xB8);
		spi_write(0x6E, 0x0F);
		spi_write(0x70, 0x02);
		spi_write(0x4A, 0x2A);
		spi_write(0x60, 0x26);
		spi_write(0x7F, 0x06);
		spi_write(0x6D, 0x70);
		spi_write(0x6E, 0x60);
		spi_write(0x6F, 0x04);
		spi_write(0x53, 0x02);
		spi_write(0x55, 0x11);
		spi_write(0x7D, 0x51);
		spi_write(0x7F, 0x08);
		spi_write(0x71, 0x4F);
		spi_write(0x7F, 0x09);
		spi_write(0x62, 0x1F);
		spi_write(0x63, 0x1F);
		spi_write(0x65, 0x03);
		spi_write(0x66, 0x03);
		spi_write(0x67, 0x1F);
		spi_write(0x68, 0x1F);
		spi_write(0x69, 0x03);
		spi_write(0x6A, 0x03);
		spi_write(0x6C, 0x1F);
		spi_write(0x6D, 0x1F);
		spi_write(0x51, 0x04);
		spi_write(0x53, 0x20);
		spi_write(0x54, 0x20);
		spi_write(0x71, 0x0F);
		spi_write(0x7F, 0x0A);
		spi_write(0x4A, 0x14);
		spi_write(0x4C, 0x14);
		spi_write(0x55, 0x19);
		spi_write(0x7F, 0x14);
		spi_write(0x63, 0x16);
		spi_write(0x7F, 0x0C);
		spi_write(0x41, 0x30);
		spi_write(0x55, 0x14);
		spi_write(0x49, 0x0A);
		spi_write(0x42, 0x00);
		spi_write(0x44, 0x0A);
		spi_write(0x5A, 0x0A);
		spi_write(0x5F, 0x1E);
		spi_write(0x5B, 0x05);
		spi_write(0x5E, 0x0F);
		spi_write(0x7F, 0x0D);
		spi_write(0x48, 0xDC);
		spi_write(0x5A, 0x29);
		spi_write(0x5B, 0x47);
		spi_write(0x5C, 0x81);
		spi_write(0x5D, 0x40);
		spi_write(0x71, 0xDC);
		spi_write(0x70, 0x07);
		spi_write(0x73, 0x00);
		spi_write(0x72, 0x08);
		spi_write(0x75, 0xDC);
		spi_write(0x74, 0x07);
		spi_write(0x77, 0x00);
		spi_write(0x76, 0x08);
		spi_write(0x7F, 0x10);
		spi_write(0x4C, 0xD0);
		spi_write(0x7F, 0x00);
		spi_write(0x4F, 0x63);
		spi_write(0x4E, 0x00);
		spi_write(0x52, 0x63);
		spi_write(0x51, 0x00);
		spi_write(0x77, 0x4F);
		spi_write(0x47, 0x01);
		spi_write(0x5B, 0x40);
		spi_write(0x66, 0x13);
		spi_write(0x67, 0x0F);
		spi_write(0x78, 0x01);
		spi_write(0x79, 0x9C);
		spi_write(0x55, 0x02);
		spi_write(0x23, 0x70);
		ss_high();
	}

	static void sensor_spi2(void)
	{
		ss_low();
		spi_write(0x7F, 0x0C);
		spi_write(0x41, 0x30);
		spi_write(0x43, 0x20);
		spi_write(0x44, 0x0D);
		spi_write(0x4A, 0x12);
		spi_write(0x4B, 0x09);
		spi_write(0x4C, 0x30);
		spi_write(0x4E, 0x08);
		spi_write(0x53, 0x16);
		spi_write(0x55, 0x14);
		spi_write(0x5A, 0x0D);
		spi_write(0x5B, 0x05);
		spi_write(0x5F, 0x1E);
		spi_write(0x66, 0x30);
		spi_write(0x7F, 0x05);
		spi_write(0x6E, 0x0F);
		spi_write(0x7F, 0x09);
		spi_write(0x71, 0x0F);
		spi_write(0x72, 0x0A);
		spi_write(0x7F, 0x00);
		spi_write(0x7F, 0x0C);
		spi_write(0x4E, 0x09);
		spi_write(0x7F, 0x00);
		spi_write(0x40, 0x80);
		spi_write(0x7F, 0x05);
		spi_write(0x4D, 0x01);
		spi_write(0x7F, 0x06);
		spi_write(0x54, 0x01);
		spi_write(0x7F, 0x00);
		spi_write(0x7F, 0x05);
		spi_write(0x44, 0x44);
		spi_write(0x7F, 0x00);
		spi_write(0x7F, 0x0D);
		spi_write(0x48, 0xDD);
		spi_write(0x7F, 0x00);
		ss_high();
	}

	static void sensor_set_dpi(const uint16_t dpi)
	{
		ss_low();
		spi_write(0x48, dpi & 0xff); // RESOLUTION_X_LOW
		spi_write(0x49, dpi >> 8); // RESOLUTION_X_HIGH
		spi_write(0x4A, dpi & 0xff); // RESOLUTION_Y_LOW
		spi_write(0x4B, dpi >> 8); //RESOLUTION_Y_HIGH
		spi_write(0x47, 0x01); // SET_RESOLUTION
		ss_high();
	}

	static void sensor_set_as(const uint8_t angle_snap)
	{
		ss_low();
		spi_write(0x56, (angle_snap << 7) | 0x0D);
		ss_high();
	}

	static void sensor_set_lod(const uint8_t lod)
	{
		ss_low();
		spi_write(0x7F, 0x0C);
		spi_write(0x4E, 0x08 | lod);
		spi_write(0x7F, 0x00);
		ss_high();
	}

	static void sensor_init(const Config cfg)
	{
		const uint16_t dpi = _FLD2VAL(CONFIG_DPI, cfg);
		const uint8_t ang_snap = (cfg & CONFIG_ANGLE_SNAP_ON) ? 1 : 0;
		const uint8_t lod = _FLD2VAL(CONFIG_LOD, cfg);

		NRESET_GPIO_CLK_ENABLE();
		__NOP();__NOP();__NOP();__NOP(); // probably unnecessary
		MODIFY_REG(NRESET_PORT->MODER,
				0b11 << (2*NRESET_PIN_Pos),
				0b01 << (2*NRESET_PIN_Pos));
		delay_ms(10);
		NRESET_PORT->ODR |= NRESET_PIN; // drive NRESET high
		delay_ms(10);
		ss_low();
		delay_ms(1);
		NRESET_PORT->ODR &= ~NRESET_PIN; // drive NRESET low
		delay_ms(1);
		NRESET_PORT->ODR |= NRESET_PIN; // drive NRESET high
		delay_ms(1);
		spi_write(0x3A, 0x5A); // 6.1.4
		ss_high();
		delay_ms(10); // 6.1.5

		// 6.1.6
		sensor_spi1();
		// 6.2.100-107
		ss_low();
		spi_write(0x22, 0x01);
		ss_high();
		int i;
		for (i = 0; i < 60; i++) {
			ss_high();
			delay_us(992); // ideally, tune this and/or use dedicated timer or systick
			ss_low();
			if (spi_read(0x6C) == 0x80) break;
		}
		ss_low();
		if (i == 60) {
			spi_write(0x7C, 0x14);
			spi_write(0x6C, 0x00);
			spi_write(0x7F, 0x00);
		}
		spi_write(0x22, 0x00);
		spi_write(0x55, 0x00);
		spi_write(0x7F, 0x00);
		spi_write(0x40, 0x00);
		ss_high();

		// equivalent of 7.3
		sensor_spi2();

		// new placement for anti-jitter configuration
		ss_low();
		spi_write(0x7f, 0x05);
		spi_write(0x43, 0x64);
		spi_write(0x7f, 0x00);
		ss_high();

		// 6.1.7
		ss_low();
		(void)spi_read(0x02);
		(void)spi_read(0x03);
		(void)spi_read(0x04);
		(void)spi_read(0x05);
		(void)spi_read(0x06);
		ss_high();

		// ???
		ss_low();
		spi_write(0x68, 0x01);
		ss_high();

		sensor_set_dpi(dpi);
		sensor_set_as(ang_snap);
		sensor_set_lod(lod);

		// invert x
		ss_low();
		spi_write(0x5B, 0x20);
		ss_high();

		// ???
		delay_us(3000); // in ***, there is a 383ms delay
		ss_low();
		spi_write(0x7F, 0x0D);
		spi_write(0x48, 0xDC);
		spi_write(0x7F, 0x00);
		ss_high();
	}
#else
	#error "Board must be defined. Board definitions are in board.h. BOARD_M2K or BOARD_M3K"
#endif
