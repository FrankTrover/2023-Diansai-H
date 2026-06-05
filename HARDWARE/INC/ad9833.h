#ifndef __AD9833_H
#define __AD9833_H

#include "main.h"
#include <stdbool.h>
#include <stdint.h>

/*
 * AD9833 DDS 驱动（STM32F407 HAL）
 *
 * 引脚映射（CubeMX 实际配置）：
 *   FSYNC: PA2 (AD9833_FSYNC)
 *   SCLK:  PA4 (AD9833_SCLK)
 *   SDATA: PA6 (AD9833_SDATA)
 *
 * AD9833 P2 接口只引出 FSYNC/SCK/SDATA，FS/PS/RST/MCLK 由板载电路处理
 * AD9833 主钟由板载晶振提供，无需 STM32 输出 MCLK
 * AD9833 最高输出频率：12.5 MHz（25 MHz 主钟下）
 */

#define AD9833_SYSTEM_CLOCK     25000000UL
#define AD9833_MAX_OUTPUT_FREQ  (AD9833_SYSTEM_CLOCK / 2UL)

/* 寄存器地址 */
#define AD9833_REG_CMD      (0U << 14)
#define AD9833_REG_FREQ0    (1U << 14)
#define AD9833_REG_FREQ1    (2U << 14)
#define AD9833_REG_PHASE0   (6U << 13)
#define AD9833_REG_PHASE1   (7U << 13)

/* 命令控制位 */
#define AD9833_B28          (1U << 13)
#define AD9833_HLB          (1U << 12)
#define AD9833_FSEL0        (0U << 11)
#define AD9833_FSEL1        (1U << 11)
#define AD9833_PSEL0        (0U << 10)
#define AD9833_PSEL1        (1U << 10)
#define AD9833_PIN_SW       (1U << 9)
#define AD9833_RESET        (1U << 8)
#define AD9833_SLEEP1       (1U << 7)
#define AD9833_SLEEP12      (1U << 6)
#define AD9833_OPBITEN      (1U << 5)
#define AD9833_SIGN_PIB     (1U << 4)
#define AD9833_DIV2         (1U << 3)
#define AD9833_MODE         (1U << 1)

/* 波形类型 */
#define AD9833_OUT_SINUS    ((0U << 5) | (0U << 1) | (0U << 3))
#define AD9833_OUT_TRIANGLE ((0U << 5) | (1U << 1) | (0U << 3))
#define AD9833_OUT_MSB      ((1U << 5) | (0U << 1) | (1U << 3))
#define AD9833_OUT_MSB2     ((1U << 5) | (0U << 1) | (0U << 3))

typedef enum
{
    AD9833_WAVEFORM_SINE = 0,
    AD9833_WAVEFORM_TRIANGLE,
    AD9833_WAVEFORM_SQUARE
} AD9833_Waveform_t;

/* 引脚映射 — 直接引用 CubeMX Label（main.h） */
#define AD9833_FSYNC_PORT   AD9833_FSYNC_GPIO_Port
#define AD9833_FSYNC_PIN    AD9833_FSYNC_Pin

#define AD9833_SCLK_PORT    AD9833_SCLK_GPIO_Port
#define AD9833_SCLK_PIN     AD9833_SCLK_Pin

#define AD9833_SDATA_PORT   AD9833_SDATA_GPIO_Port
#define AD9833_SDATA_PIN    AD9833_SDATA_Pin

/* 引脚控制宏 */
#define AD9833_FSYNC_SET()  HAL_GPIO_WritePin(AD9833_FSYNC_PORT, AD9833_FSYNC_PIN, GPIO_PIN_SET)
#define AD9833_FSYNC_CLR()  HAL_GPIO_WritePin(AD9833_FSYNC_PORT, AD9833_FSYNC_PIN, GPIO_PIN_RESET)

#define AD9833_SCLK_SET()   HAL_GPIO_WritePin(AD9833_SCLK_PORT, AD9833_SCLK_PIN, GPIO_PIN_SET)
#define AD9833_SCLK_CLR()   HAL_GPIO_WritePin(AD9833_SCLK_PORT, AD9833_SCLK_PIN, GPIO_PIN_RESET)

#define AD9833_SDATA_SET()  HAL_GPIO_WritePin(AD9833_SDATA_PORT, AD9833_SDATA_PIN, GPIO_PIN_SET)
#define AD9833_SDATA_CLR()  HAL_GPIO_WritePin(AD9833_SDATA_PORT, AD9833_SDATA_PIN, GPIO_PIN_RESET)

void AD9833_Init(void);
void AD9833_Write_16Bits(uint16_t data);
void AD9833_SetRegisterValue(uint16_t regValue);
void AD9833_SetFrequency(uint16_t reg, float fout);
void AD9833_SetPhase(uint16_t reg, uint16_t val);
void AD9833_Setup(uint16_t freq, uint16_t phase, uint16_t type);
void AD9833_SetFrequencyQuick(float fout, uint16_t type);
bool AD9833_ConfigureOutput(AD9833_Waveform_t waveform, float freq_hz);

#endif /* __AD9833_H */
