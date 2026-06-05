#include "signal_output.h"

#include "Communication.h"
#include "ad9833.h"
#include "ad9834.h"

static float g_last_vpp = SIGNAL_OUTPUT_MIN_VPP;

void Signal_Output_Init(void)
{
    /* 初始化共享 SPI 引脚 */
    DDS_GPIO_Init();

    /* 初始化两片 DDS */
    AD9833_Init();
    AD9834_Init();

    /* 默认输出：AD9833 输出 60kHz 正弦，AD9834 输出 100kHz 正弦 */
    AD9833_ConfigureOutput(AD9833_WAVEFORM_SINE, 60000.0f);
    AD9834_ConfigureOutput(AD9834_WAVEFORM_SINE, 100000U);

    g_last_vpp = SIGNAL_OUTPUT_MIN_VPP;
}

bool Set_Waveform_Output(uint32_t freq, uint8_t wave_type, float vpp)
{
    /*
     * 第一路 DDS 输出（信号 A'）：始终使用 AD9833。
     * AD9833 主钟 25MHz，最高输出 12.5MHz，覆盖 20-100kHz 全部场景。
     */
    AD9833_Waveform_t ad9833_wave;

    g_last_vpp = vpp;

    switch (wave_type)
    {
        case DSP_WAVE_SINE:
            ad9833_wave = AD9833_WAVEFORM_SINE;
            break;
        case DSP_WAVE_TRIANGLE:
            ad9833_wave = AD9833_WAVEFORM_TRIANGLE;
            break;
        case DSP_WAVE_SQUARE:
            ad9833_wave = AD9833_WAVEFORM_SQUARE;
            break;
        default:
            ad9833_wave = AD9833_WAVEFORM_SINE;
            break;
    }

    return AD9833_ConfigureOutput(ad9833_wave, (float)freq);
}

bool Set_Waveform_Output_Dual(uint32_t freq, uint8_t wave_type, float vpp)
{
    /*
     * 第二路 DDS 输出（信号 B'）：始终使用 AD9834。
     * AD9834 主钟 75MHz，最高输出 37.5MHz，覆盖 20-100kHz 全部场景。
     */
    AD9834_Waveform_t ad9834_wave;

    (void)vpp;

    switch (wave_type)
    {
        case DSP_WAVE_SINE:
            ad9834_wave = AD9834_WAVEFORM_SINE;
            break;
        case DSP_WAVE_TRIANGLE:
            ad9834_wave = AD9834_WAVEFORM_TRIANGLE;
            break;
        case DSP_WAVE_SQUARE:
            ad9834_wave = AD9834_WAVEFORM_SQUARE;
            break;
        default:
            ad9834_wave = AD9834_WAVEFORM_SINE;
            break;
    }

    return AD9834_ConfigureOutput(ad9834_wave, freq);
}

float Signal_Output_GetLastVpp(void)
{
    return g_last_vpp;
}