#include "ad9833.h"

/*
 * AD9833 DDS 驱动源文件
 *
 * 协议时序：GPIO 软件模拟 SPI，16-bit 写入
 * SPI GPIO 由 Communication 层 DDS_SharedSPI_Init() 统一初始化
 * AD9833 P2 接口只引出 FSYNC/SCK/SDATA，FS/PS/RST/MCLK 由板载电路处理
 *
 * 频率计算公式：Fout = (FREQREG * FCLK) / 2^28
 *   即 FREQREG = Fout * 2^28 / FCLK
 */

#define REAL_FRE_DAT  (268435456.0f / (float)AD9833_SYSTEM_CLOCK)

static uint16_t AD9833_BuildControlWord(uint16_t freq, uint16_t phase, uint16_t type, bool reset)
{
    uint16_t control = (uint16_t)(AD9833_B28 | type);

    if (freq == AD9833_REG_FREQ1)
    {
        control |= AD9833_FSEL1;
    }

    if (phase == AD9833_REG_PHASE1)
    {
        control |= AD9833_PSEL1;
    }

    if (reset)
    {
        control |= AD9833_RESET;
    }

    return control;
}

static void AD9833_WriteFrequencyWords(uint16_t reg, float fout)
{
    uint16_t freqHi = (uint16_t)reg;
    uint16_t freqLo = (uint16_t)reg;
    uint32_t val = (uint32_t)((REAL_FRE_DAT * fout) + 0.5f);

    freqHi |= (uint16_t)((val & 0xFFFC000U) >> 14);
    freqLo |= (uint16_t)(val & 0x3FFFU);

    AD9833_SetRegisterValue(freqLo);
    AD9833_SetRegisterValue(freqHi);
}

void AD9833_Init(void)
{
    /* GPIO 已由 DDS_SharedSPI_Init() 初始化，这里只做复位 */
    AD9833_SetRegisterValue(AD9833_B28 | AD9833_RESET);
}

void AD9833_Write_16Bits(uint16_t data)
{
    uint8_t i;

    AD9833_SCLK_SET();
    AD9833_FSYNC_CLR();

    for (i = 0; i < 16; i++)
    {
        if (data & 0x8000U)
            AD9833_SDATA_SET();
        else
            AD9833_SDATA_CLR();

        AD9833_SCLK_CLR();
        data <<= 1;
        AD9833_SCLK_SET();
    }

    AD9833_SDATA_SET();
    AD9833_FSYNC_SET();
}

void AD9833_SetRegisterValue(uint16_t regValue)
{
    AD9833_Write_16Bits(regValue);
}

void AD9833_SetFrequency(uint16_t reg, float fout)
{
    AD9833_SetRegisterValue(AD9833_B28);
    AD9833_WriteFrequencyWords(reg, fout);
}

void AD9833_SetPhase(uint16_t reg, uint16_t val)
{
    uint16_t phase = (uint16_t)reg;
    phase |= val;
    AD9833_SetRegisterValue(phase);
}

void AD9833_Setup(uint16_t freq, uint16_t phase, uint16_t type)
{
    AD9833_SetRegisterValue(AD9833_BuildControlWord(freq, phase, type, false));
}

void AD9833_SetFrequencyQuick(float fout, uint16_t type)
{
    AD9833_SetRegisterValue(AD9833_BuildControlWord(AD9833_REG_FREQ0, AD9833_REG_PHASE0, type, true));
    AD9833_WriteFrequencyWords(AD9833_REG_FREQ0, fout);
    AD9833_SetPhase(AD9833_REG_PHASE0, 0U);
    AD9833_Setup(AD9833_REG_FREQ0, AD9833_REG_PHASE0, type);
}

bool AD9833_ConfigureOutput(AD9833_Waveform_t waveform, float freq_hz)
{
    uint16_t wave_cmd;

    if (freq_hz < 0.0f || freq_hz > (float)AD9833_MAX_OUTPUT_FREQ)
    {
        return false;
    }

    switch (waveform)
    {
        case AD9833_WAVEFORM_SINE:
            wave_cmd = AD9833_OUT_SINUS;
            break;
        case AD9833_WAVEFORM_TRIANGLE:
            wave_cmd = AD9833_OUT_TRIANGLE;
            break;
        case AD9833_WAVEFORM_SQUARE:
            wave_cmd = AD9833_OUT_MSB;
            break;
        default:
            return false;
    }

    /* Hold reset while updating the frequency and phase registers. */
    AD9833_SetRegisterValue(AD9833_BuildControlWord(AD9833_REG_FREQ0, AD9833_REG_PHASE0, wave_cmd, true));

    /* Write frequency tuning word. */
    AD9833_WriteFrequencyWords(AD9833_REG_FREQ0, freq_hz);
    AD9833_SetPhase(AD9833_REG_PHASE0, 0U);

    /* Select waveform and release reset. */
    AD9833_Setup(AD9833_REG_FREQ0, AD9833_REG_PHASE0, wave_cmd);

    return true;
}
