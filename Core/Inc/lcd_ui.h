#ifndef __LCD_UI_H__
#define __LCD_UI_H__

#include "dsp_process.h"

typedef struct
{
    bool valid;
    uint32_t freq_hz;
    DSP_WaveformType_t wave_type;
} LCD_SignalInfo_t;

typedef struct
{
    bool valid;
    LCD_SignalInfo_t input_a;
    LCD_SignalInfo_t input_b;
    LCD_SignalInfo_t dds_a;
    LCD_SignalInfo_t dds_b;
} LCD_SplitResult_t;

void LCD_UI_Init(void);
void LCD_UI_ShowIdle(void);
void LCD_UI_ShowAcquiring(void);
void LCD_UI_ShowAnalysis(const SignalAnalysisResult_t *result, float output_vpp);
void LCD_UI_ShowSplitResult(const LCD_SplitResult_t *result);
void LCD_UI_ShowError(const char *message);

#endif