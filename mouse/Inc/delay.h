#pragma once

#include "stm32f7xx.h"

// TIM2CLK is based on APB1
// APB1 prescaler = 1 = PCLK1/APB1
// APB1 prescaler > 1 = 2 x PCLK1/APB1

#ifdef BOARD_M2K
    #define TICKS_PER_US 64U // TIMCLK2 = 128 / 4 = 32 MHz x 2 = 64 MHz
#elif BOARD_M3K
    #define TICKS_PER_US 80U // TIMCLK2 = 160 / 4 = 40 MHz x 2 = 80 MHz
#endif

void delay_init(void);

#define DELAY_SLEEP

#ifdef DELAY_SLEEP
    static inline void delay_us(const uint32_t us)
    {
        TIM2->CNT = TICKS_PER_US*us - 1;
        TIM2->CR1 = TIM_CR1_CEN | TIM_CR1_DIR;
        while (TIM2->CR1 != 0) // can comment if only interrupt is TIM2_IRQHandler
            __WFI();
    }
#else // delay with busy wait
    static inline void delay_us(const uint32_t us)
    {
        TIM2->CNT = 0;
        while (TIM2->CNT < TICKS_PER_US*us);
    }
#endif

#define delay_ms(x) delay_us(1000*(x))
