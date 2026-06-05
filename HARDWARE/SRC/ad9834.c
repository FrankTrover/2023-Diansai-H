#include "ad9834.h"

/*
 * AD9834 DDS driver.
 *
 * SPI GPIO is initialized by DDS_SharedSPI_Init() in Communication.c.
 * This file only handles AD9834 register writes and output selection.
 */

void AD9834_Write_16Bits(uint16_t data)
{
    uint8_t i;

    AD9834_SCLK_SET();
    AD9834_FSYNC_CLR();

    for (i = 0; i < 16; i++)
    {
        if (data & 0x8000U)
            AD9834_SDATA_SET();
        else
            AD9834_SDATA_CLR();

        AD9834_SCLK_CLR();
        data <<= 1;
        AD9834_SCLK_SET();
    }

    AD9834_SDATA_SET();
    AD9834_FSYNC_SET();
}

void AD9834_Select_Wave(uint16_t wave_type)
{
    AD9834_FSYNC_SET();
    AD9834_SCLK_SET();

    AD9834_Write_16Bits(wave_type);
}

void AD9834_Init(void)
{
    /* GPIO is already initialized by DDS_SharedSPI_Init(). */
    AD9834_FS_LOW();
    AD9834_PS_LOW();

    /*
     * Keep RESET asserted after init. AD9834_ConfigureOutput() releases RESET
     * only after the frequency word and final waveform control word are written.
     */
    AD9834_Write_16Bits(0x2100U);  /* B28=1, RESET=1 */
    AD9834_Write_16Bits(0xC000U);  /* PHASE0=0 */
    AD9834_Write_16Bits(0x2100U);  /* B28=1, RESET=1 */
}

void AD9834_Set_Freq(uint8_t freq_number, uint32_t freq)
{
    uint32_t FREQREG = (uint32_t)(((((uint64_t)freq) << 28) + (AD9834_SYSTEM_CLOCK / 2U)) / AD9834_SYSTEM_CLOCK);
    uint16_t FREQREG_LSB_14BIT;
    uint16_t FREQREG_MSB_14BIT;

    FREQREG &= 0x0FFFFFFFU;
    FREQREG_LSB_14BIT = (uint16_t)(FREQREG & 0x3FFFU);
    FREQREG_MSB_14BIT = (uint16_t)((FREQREG >> 14) & 0x3FFFU);

    if (freq_number == AD9834_FREQ0)
    {
        FREQREG_LSB_14BIT |= 0x4000U;  /* D15=0, D14=1 -> FREQ0 */
        FREQREG_MSB_14BIT |= 0x4000U;
    }
    else
    {
        FREQREG_LSB_14BIT |= 0x8000U;  /* D15=1, D14=0 -> FREQ1 */
        FREQREG_MSB_14BIT |= 0x8000U;
    }

    AD9834_Write_16Bits(FREQREG_LSB_14BIT);
    AD9834_Write_16Bits(FREQREG_MSB_14BIT);
}

bool AD9834_ConfigureOutput(AD9834_Waveform_t waveform, uint32_t freq_hz)
{
    uint16_t wave_word;

    if (freq_hz > AD9834_MAX_OUTPUT_FREQ)
    {
        return false;
    }

    switch (waveform)
    {
        case AD9834_WAVEFORM_SINE:
            wave_word = AD9834_WAVE_SINE;
            break;
        case AD9834_WAVEFORM_TRIANGLE:
            wave_word = AD9834_WAVE_TRIANGLE;
            break;
        case AD9834_WAVEFORM_SQUARE:
            wave_word = AD9834_WAVE_SQUARE;
            break;
        default:
            return false;
    }

    /* Hold reset while updating the 28-bit frequency word. */
    AD9834_Write_16Bits(0x2100U);
    AD9834_Set_Freq(AD9834_FREQ0, freq_hz);

    /* Select the target waveform and release reset. */
    AD9834_Select_Wave(wave_word);

    return true;
}
