#ifndef __DELAY_H
#define __DELAY_H

#include "stm32f4xx_hal.h"

void delay_init(uint32_t sysclk_freq);
void delay_us(uint32_t us);
void delay_ms(uint32_t ms);
void delay_ns(uint32_t ns);

#endif