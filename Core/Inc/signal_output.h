#ifndef __SIGNAL_OUTPUT_H__
#define __SIGNAL_OUTPUT_H__

#include <stdbool.h>
#include <stdint.h>

#include "dsp_process.h"

#define SIGNAL_OUTPUT_MIN_VPP         1.0f
#define SIGNAL_OUTPUT_FULL_SCALE_VPP  2.0f

void Signal_Output_Init(void);
bool Set_Waveform_Output(uint32_t freq, uint8_t wave_type, float vpp);
bool Set_Waveform_Output_Dual(uint32_t freq, uint8_t wave_type, float vpp);
float Signal_Output_GetLastVpp(void);

#endif