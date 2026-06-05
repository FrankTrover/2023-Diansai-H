#ifndef _COMMUNICATION_H_
#define _COMMUNICATION_H_

#include "main.h"
#include <stdbool.h>
#include <stdint.h>

/*
 * Communication 驱动
 *
 * AD9833 独立 SPI（GPIOA）：FSYNC(PA2), SCLK(PA4), SDATA(PA6)
 * AD9834 独立 SPI（GPIOE）：FSY(PE2), SCK(PE4), SDA(PE6)
 * 两片 DDS 没有共享 SPI 线，各自独立总线
 * MCLK 由两片 DDS 板载晶振提供，无需 STM32 输出
 */

/* 初始化两片 DDS 的全部 GPIO */
void DDS_GPIO_Init(void);

#endif /* _COMMUNICATION_H_ */
