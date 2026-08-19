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

#include "stm32f7xx.h"

/*
STM32F722ic/STM32F730r8 Datasheets: Table 16. General operating conditions
  0 - 144 MHz power scale 3 PWR_CR1_VOS = 0b01, overdrive off
  0 - 168 MHz power scale 2 PWR_CR1_VOS = 0b10, overdrive off
169 - 180 MHz power scale 2 PWR CR1_VOS = 0b10, overdrive  on
  0 - 180 MHz power scale 1 PWR CR1_VOS = 0b11, overdrive off
181 - 216 MHz power scale 1 PWR CR1_VOS = 0b11, overdrive  on

RM0431: Table 5. Number of wait states according to CPU clock (HCLK frequency)
VDD 2.7 V - 3.6 V
  0 -  30 MHz = 0 WS (1 CPU cycles)
 31 -  60 MHz = 1 WS (2 CPU cycles)
 61 -  90 MHz = 2 WS (3 CPU cycles)
 91 - 120 MHz = 3 WS (4 CPU cycles)
121 - 150 MHz = 4 WS (5 CPU cycles)
151 - 180 MHz = 5 WS (6 CPU cycles)
181 - 210 MHz = 6 WS (7 CPU cycles)
211 - 216 MHz = 7 WS (8 CPU cycles)
*/

#ifdef BOARD_M2K
// M2K SYSCLK 128 MHz SPI 2 MHz
static void clk_init(void)
{
    RCC->CR |= RCC_CR_HSEON; // turn on HSE (24 MHz)
    while ((RCC->CR & RCC_CR_HSERDY) == 0);

    RCC->CR &= ~RCC_CR_PLLON; // disable PLL
    while ((RCC->CR & RCC_CR_PLLRDY) != 0);

    RCC->APB1ENR |= RCC_APB1ENR_PWREN;
    MODIFY_REG(PWR->CR1, PWR_CR1_VOS, _VAL2FLD(PWR_CR1_VOS, 0b01)); // Scale 3 for 128 MHz

    MODIFY_REG(FLASH->ACR, FLASH_ACR_LATENCY, FLASH_ACR_LATENCY_4WS); // 4WS for 128 MHz
    (void)FLASH->ACR; // Guarantees that flash latency has been set before changing the sysclk

    // Main PLL 128 MHz highest we can do and get 2 MHz SPI which is amx for PMW3360
    MODIFY_REG(RCC->PLLCFGR,
         RCC_PLLCFGR_PLLM | RCC_PLLCFGR_PLLN | RCC_PLLCFGR_PLLP | RCC_PLLCFGR_PLLSRC | RCC_PLLCFGR_PLLQ,
         _VAL2FLD(RCC_PLLCFGR_PLLM, 12) | _VAL2FLD(RCC_PLLCFGR_PLLN, 128) | _VAL2FLD(RCC_PLLCFGR_PLLP, 0b00) | // PLLP = 2
         RCC_PLLCFGR_PLLSRC_HSE | _VAL2FLD(RCC_PLLCFGR_PLLQ, 8) // PLLQ = 8 (32 MHz, unused for USB)
    );
    RCC->CR |= RCC_CR_PLLON; // enable PLL
    while ((RCC->CR & RCC_CR_PLLRDY) == 0);

    // PLLSAI for 48 MHz USB
    MODIFY_REG(RCC->PLLSAICFGR,
        RCC_PLLSAICFGR_PLLSAIN | RCC_PLLSAICFGR_PLLSAIQ,
        _VAL2FLD(RCC_PLLSAICFGR_PLLSAIN, 168) | _VAL2FLD(RCC_PLLSAICFGR_PLLSAIQ, 7) // 336 MHz / 7 = 48 MHz
    );
    RCC->CR |= RCC_CR_PLLSAION;
    while ((RCC->CR & RCC_CR_PLLSAIRDY) == 0);
    RCC->DCKCFGR2 |= RCC_DCKCFGR2_CK48MSEL; // Select PLLSAI_Q for 48 MHz USB

    // Before switch SYSCLK to PLL lower APB so we don't run them higher than they should be
    MODIFY_REG(RCC->CFGR, RCC_CFGR_PPRE1, RCC_CFGR_PPRE1_DIV16);
    MODIFY_REG(RCC->CFGR, RCC_CFGR_PPRE2, RCC_CFGR_PPRE2_DIV16);

    // Switch SYSCLK to PLL
    MODIFY_REG(RCC->CFGR, RCC_CFGR_HPRE, RCC_CFGR_HPRE_DIV1); // SYSCLK/AHB = 128 MHz
    MODIFY_REG(RCC->CFGR, RCC_CFGR_SW, RCC_CFGR_SW_PLL);
    while ((RCC->CFGR & RCC_CFGR_SWS) != RCC_CFGR_SWS_PLL);

    // Set APB to the correct dividers based on our PLL SYSCLK
    MODIFY_REG(RCC->CFGR, RCC_CFGR_PPRE1, RCC_CFGR_PPRE1_DIV4); // PCLK1/APB1 = 32 MHz
    MODIFY_REG(RCC->CFGR, RCC_CFGR_PPRE2, RCC_CFGR_PPRE2_DIV2); // PCLK2/APB2 = 64 MHz
    RCC->CR &= ~RCC_CR_HSION; // turn off HSI
}

// 160 MHz
#elif BOARD_M3K
static void clk_init(void)
{
    RCC->CR |= RCC_CR_HSEON; // Turn on HSE (24 MHz)
    while ((RCC->CR & RCC_CR_HSERDY) == 0);

    RCC->CR &= ~RCC_CR_PLLON; // Disable PLL
    while ((RCC->CR & RCC_CR_PLLRDY) != 0);

    RCC->APB1ENR |= RCC_APB1ENR_PWREN;
    MODIFY_REG(PWR->CR1, PWR_CR1_VOS, _VAL2FLD(PWR_CR1_VOS, 0b11)); // Scale 1 for 160 MHz

    MODIFY_REG(FLASH->ACR, FLASH_ACR_LATENCY, FLASH_ACR_LATENCY_5WS); // 5WS for 160 MHz
    (void)FLASH->ACR; // Guarantees that flash latency has been set before changing the sysclk

    // Main PLL 160 MHz highest we can do and get 10 MHz SPI which is max for PAW3399
    MODIFY_REG(RCC->PLLCFGR,
        RCC_PLLCFGR_PLLM | RCC_PLLCFGR_PLLN | RCC_PLLCFGR_PLLP | RCC_PLLCFGR_PLLSRC | RCC_PLLCFGR_PLLQ,
        _VAL2FLD(RCC_PLLCFGR_PLLM, 12) | _VAL2FLD(RCC_PLLCFGR_PLLN, 160) | _VAL2FLD(RCC_PLLCFGR_PLLP, 0b00) | // PLLP = 2
        RCC_PLLCFGR_PLLSRC_HSE | _VAL2FLD(RCC_PLLCFGR_PLLQ, 8) // PLLQ = 8 (40 MHz, unused for USB)
    );
    RCC->CR |= RCC_CR_PLLON;
    while ((RCC->CR & RCC_CR_PLLRDY) == 0);

    // PLLSAI for 48 MHz USB
    MODIFY_REG(RCC->PLLSAICFGR,
        RCC_PLLSAICFGR_PLLSAIN | RCC_PLLSAICFGR_PLLSAIQ,
        _VAL2FLD(RCC_PLLSAICFGR_PLLSAIN, 168) | _VAL2FLD(RCC_PLLSAICFGR_PLLSAIQ, 7) // 336 MHz / 7 = 48 MHz
    );
    RCC->CR |= RCC_CR_PLLSAION;
    while ((RCC->CR & RCC_CR_PLLSAIRDY) == 0);
    RCC->DCKCFGR2 |= RCC_DCKCFGR2_CK48MSEL; // Select PLLSAI_Q for 48 MHz USB

    // Before switch SYSCLK to PLL lower APB so we don't run them higher than they should be
    MODIFY_REG(RCC->CFGR, RCC_CFGR_PPRE1, RCC_CFGR_PPRE1_DIV16);
    MODIFY_REG(RCC->CFGR, RCC_CFGR_PPRE2, RCC_CFGR_PPRE2_DIV16);

    // Switch SYSCLK to PLL
    MODIFY_REG(RCC->CFGR, RCC_CFGR_HPRE, RCC_CFGR_HPRE_DIV1); // SYSCLOCK/AHB = 160 MHz
    MODIFY_REG(RCC->CFGR, RCC_CFGR_SW, RCC_CFGR_SW_PLL);
    while ((RCC->CFGR & RCC_CFGR_SWS) != RCC_CFGR_SWS_PLL);

    // Set APB to the correct dividers based on our PLL SYSCLK
    MODIFY_REG(RCC->CFGR, RCC_CFGR_PPRE1, RCC_CFGR_PPRE1_DIV4); // PCLK1/APB1 = 40 MHz
    MODIFY_REG(RCC->CFGR, RCC_CFGR_PPRE2, RCC_CFGR_PPRE2_DIV4); // PCLK2/APB2 = 40 MHz
    RCC->CR &= ~RCC_CR_HSION; // turn off HSI
}

#endif
