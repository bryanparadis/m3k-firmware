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

// 32
#if 1
static void clk_init(void)
{
	RCC->CR |= RCC_CR_HSEON; // turn on HSE
    while ((RCC->CR & RCC_CR_HSERDY) == 0);

    RCC->CR &= ~RCC_CR_PLLON; // disable PLL (off by default, not necessary)
    while ((RCC->CR & RCC_CR_PLLRDY) != 0);

    RCC->APB1ENR |= RCC_APB1ENR_PWREN;
    MODIFY_REG(PWR->CR1, PWR_CR1_VOS, _VAL2FLD(PWR_CR1_VOS, 0b01)); //scale 3 power

    MODIFY_REG(FLASH->ACR, FLASH_ACR_LATENCY, FLASH_ACR_LATENCY_1WS); // flash latency for 32MHz
	(void)FLASH->ACR; // Guarantees that flash latency has been set before changing the sysclk

    // configure PLL for 32MHz sysclk
    MODIFY_REG(RCC->PLLCFGR,
         RCC_PLLCFGR_PLLM | RCC_PLLCFGR_PLLN | RCC_PLLCFGR_PLLP | RCC_PLLCFGR_PLLSRC | RCC_PLLCFGR_PLLQ,
         _VAL2FLD(RCC_PLLCFGR_PLLM, 12) | _VAL2FLD(RCC_PLLCFGR_PLLN, 96) | _VAL2FLD(RCC_PLLCFGR_PLLP, 0b10) |  // for P = 6
		 RCC_PLLCFGR_PLLSRC_HSE | _VAL2FLD(RCC_PLLCFGR_PLLQ, 4)
    );
    RCC->CR |= RCC_CR_PLLON; // enable PLL
    while ((RCC->CR & RCC_CR_PLLRDY) == 0);

    /* Set the highest APBx dividers in order to ensure that we do not go through
       a non-spec phase whatever we decrease or increase HCLK. */
    MODIFY_REG(RCC->CFGR, RCC_CFGR_PPRE1, RCC_CFGR_PPRE1_DIV16);
    MODIFY_REG(RCC->CFGR, RCC_CFGR_PPRE2, RCC_CFGR_PPRE2_DIV16);

    MODIFY_REG(RCC->CFGR, RCC_CFGR_HPRE, RCC_CFGR_HPRE_DIV1); // ahb clk = sysclk
    MODIFY_REG(RCC->CFGR, RCC_CFGR_SW, RCC_CFGR_SW_PLL); // set pll as sys clock
    while ((RCC->CFGR & RCC_CFGR_SWS) != RCC_CFGR_SWS_PLL);

    MODIFY_REG(RCC->CFGR, RCC_CFGR_PPRE1, RCC_CFGR_PPRE1_DIV1); // apb1 = sysclk
    MODIFY_REG(RCC->CFGR, RCC_CFGR_PPRE2, RCC_CFGR_PPRE2_DIV1); // apb2 = sysclk
    RCC->CR &= ~RCC_CR_HSION; // turn off HSI (not necessary)
}
#endif

// 64
#if 0
static void clk_init(void)
{
    RCC->CR |= RCC_CR_HSEON; // turn on HSE (24 MHz)
    while ((RCC->CR & RCC_CR_HSERDY) == 0);

    RCC->CR &= ~RCC_CR_PLLON; // disable PLL
    while ((RCC->CR & RCC_CR_PLLRDY) != 0);

    RCC->APB1ENR |= RCC_APB1ENR_PWREN;
    MODIFY_REG(PWR->CR1, PWR_CR1_VOS, _VAL2FLD(PWR_CR1_VOS, 0b10)); // Scale 2 for 64 MHz

    MODIFY_REG(FLASH->ACR, FLASH_ACR_LATENCY, FLASH_ACR_LATENCY_2WS); // Flash latency for 64 MHz
	(void)FLASH->ACR; // Guarantees that flash latency has been set before changing the sysclk

    // Configure PLL for 64 MHz SYSCLK and 48 MHz USB clock (HSE = 24 MHz)
    MODIFY_REG(RCC->PLLCFGR,
         RCC_PLLCFGR_PLLM | RCC_PLLCFGR_PLLN | RCC_PLLCFGR_PLLP | RCC_PLLCFGR_PLLSRC | RCC_PLLCFGR_PLLQ,
         _VAL2FLD(RCC_PLLCFGR_PLLM, 12) | _VAL2FLD(RCC_PLLCFGR_PLLN, 192) | _VAL2FLD(RCC_PLLCFGR_PLLP, 0b10) | // PLLP = 6
         RCC_PLLCFGR_PLLSRC_HSE | _VAL2FLD(RCC_PLLCFGR_PLLQ, 8) // PLLQ = 8
    );
    RCC->CR |= RCC_CR_PLLON; // enable PLL
    while ((RCC->CR & RCC_CR_PLLRDY) == 0);

    /* Set the highest APBx dividers to ensure safe clock transitions */
    MODIFY_REG(RCC->CFGR, RCC_CFGR_PPRE1, RCC_CFGR_PPRE1_DIV16);
    MODIFY_REG(RCC->CFGR, RCC_CFGR_PPRE2, RCC_CFGR_PPRE2_DIV16);

    MODIFY_REG(RCC->CFGR, RCC_CFGR_HPRE, RCC_CFGR_HPRE_DIV1); // AHB = SYSCLK
    MODIFY_REG(RCC->CFGR, RCC_CFGR_SW, RCC_CFGR_SW_PLL); // Set PLL as sys clock
    while ((RCC->CFGR & RCC_CFGR_SWS) != RCC_CFGR_SWS_PLL);

    MODIFY_REG(RCC->CFGR, RCC_CFGR_PPRE1, RCC_CFGR_PPRE1_DIV1); // APB1 = SYSCLK
    MODIFY_REG(RCC->CFGR, RCC_CFGR_PPRE2, RCC_CFGR_PPRE2_DIV1); // APB2 = SYSCLK
    RCC->CR &= ~RCC_CR_HSION; // turn off HSI
}
#endif

#if 0 // 160MHz
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

    // Main PLL: SYSCLK = 160 MHz
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

    MODIFY_REG(RCC->CFGR, RCC_CFGR_PPRE1, RCC_CFGR_PPRE1_DIV16);
    MODIFY_REG(RCC->CFGR, RCC_CFGR_PPRE2, RCC_CFGR_PPRE2_DIV16);

    MODIFY_REG(RCC->CFGR, RCC_CFGR_HPRE, RCC_CFGR_HPRE_DIV1); // HCLK = 160 MHz
    MODIFY_REG(RCC->CFGR, RCC_CFGR_SW, RCC_CFGR_SW_PLL);
    while ((RCC->CFGR & RCC_CFGR_SWS) != RCC_CFGR_SWS_PLL);

    MODIFY_REG(RCC->CFGR, RCC_CFGR_PPRE1, RCC_CFGR_PPRE1_DIV4); // PCLK1 = 40 MHz
    MODIFY_REG(RCC->CFGR, RCC_CFGR_PPRE2, RCC_CFGR_PPRE2_DIV4); // PCLK2 = 40 MHz
    RCC->CR &= ~RCC_CR_HSION; // turn off HSI
}
#endif
