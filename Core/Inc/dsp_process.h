#ifndef __DSP_PROCESS_H__
#define __DSP_PROCESS_H__

#include <stdbool.h>
#include <stdint.h>

#include "stm32f4xx_hal.h"

#define DSP_FFT_SIZE              2048U
#define DSP_SPECTRUM_SIZE         (DSP_FFT_SIZE / 2U)
#define DSP_MIN_ANALYSIS_FREQ_HZ  20000.0f
#define DSP_MAX_ANALYSIS_FREQ_HZ  100000.0f

typedef enum
{
    DSP_WAVE_UNKNOWN = 0,
    DSP_WAVE_SINE,
    DSP_WAVE_TRIANGLE,
    DSP_WAVE_SQUARE
} DSP_WaveformType_t;

typedef struct
{
    bool valid;
    uint16_t peak_bin;
    float frequency_hz;
    float fundamental_amplitude;
    float third_harmonic_amplitude;
    float fifth_harmonic_amplitude;
    float harmonic_ratio;
    DSP_WaveformType_t wave_type;
} SignalComponent_t;

typedef struct
{
    bool valid;
    uint32_t sample_count;
    float sample_rate_hz;
    const float *spectrum;
    uint32_t spectrum_bins;
    SignalComponent_t primary;
    SignalComponent_t secondary;
} SignalAnalysisResult_t;

void DSP_Process_Init(void);
HAL_StatusTypeDef DSP_ProcessSamples(const int16_t *samples, uint32_t sample_count, float sample_rate_hz);
const SignalAnalysisResult_t *DSP_GetLastAnalysis(void);
const float *DSP_GetSpectrum(void);
const char *DSP_WaveTypeToString(DSP_WaveformType_t wave_type);

#endif
