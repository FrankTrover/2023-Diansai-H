#include "dsp_process.h"

#include <math.h>
#include <stddef.h>
#include <string.h>

#include "arm_const_structs.h"
#include "arm_math.h"

#define DSP_PI                 3.14159265358979323846f
#define DSP_PEAK_GUARD_BINS    3U
#define DSP_HARMONIC_SEARCH_BINS 6U
#define DSP_HANN_COHERENT_GAIN 0.5f
#define DSP_SECONDARY_MIN_RATIO 0.05f
#define DSP_H5_COLLISION_TRI_RATIO 0.075f

static float g_fft_buffer[2U * DSP_FFT_SIZE];
static float g_spectrum[DSP_SPECTRUM_SIZE];
static SignalAnalysisResult_t g_last_analysis;

static float dsp_hann_window(uint32_t index)
{
    if (DSP_FFT_SIZE <= 1U)
    {
        return 1.0f;
    }

    return 0.5f - 0.5f * cosf((2.0f * DSP_PI * (float)index) / (float)(DSP_FFT_SIZE - 1U));
}

static float dsp_bin_resolution(float sample_rate_hz)
{
    if (sample_rate_hz <= 0.0f)
    {
        return 0.0f;
    }

    return sample_rate_hz / (float)DSP_FFT_SIZE;
}

static uint32_t dsp_frequency_to_bin(float frequency_hz, float sample_rate_hz)
{
    float resolution = dsp_bin_resolution(sample_rate_hz);

    if (resolution <= 0.0f)
    {
        return 0U;
    }

    return (uint32_t)(frequency_hz / resolution);
}

static uint32_t dsp_frequency_to_nearest_bin(float frequency_hz, float sample_rate_hz)
{
    float resolution = dsp_bin_resolution(sample_rate_hz);

    if (resolution <= 0.0f)
    {
        return 0U;
    }

    return (uint32_t)((frequency_hz / resolution) + 0.5f);
}

static float dsp_fold_frequency(float frequency_hz, float sample_rate_hz)
{
    float folded;
    float nyquist;

    if ((frequency_hz <= 0.0f) || (sample_rate_hz <= 0.0f))
    {
        return 0.0f;
    }

    folded = fmodf(frequency_hz, sample_rate_hz);
    if (folded < 0.0f)
    {
        folded = -folded;
    }

    nyquist = sample_rate_hz * 0.5f;
    if (folded > nyquist)
    {
        folded = sample_rate_hz - folded;
    }

    return folded;
}

static uint32_t dsp_find_peak(const float *spectrum,
                              uint32_t start_bin,
                              uint32_t end_bin,
                              int32_t exclude_center)
{
    uint32_t best_bin = 0U;
    float best_value = 0.0f;
    uint32_t bin;

    for (bin = start_bin; bin <= end_bin; ++bin)
    {
        if ((exclude_center >= 0) && ((uint32_t)exclude_center >= DSP_PEAK_GUARD_BINS))
        {
            uint32_t exclude_start = (uint32_t)exclude_center - DSP_PEAK_GUARD_BINS;
            uint32_t exclude_end = (uint32_t)exclude_center + DSP_PEAK_GUARD_BINS;

            if ((bin >= exclude_start) && (bin <= exclude_end))
            {
                continue;
            }
        }

        if (spectrum[bin] > best_value)
        {
            best_value = spectrum[bin];
            best_bin = bin;
        }
    }

    return best_bin;
}

static float dsp_parabolic_interp(const float *spectrum, uint32_t peak_bin)
{
    float alpha, beta, gamma, delta;

    if ((peak_bin == 0U) || (peak_bin >= DSP_SPECTRUM_SIZE - 1U))
    {
        return (float)peak_bin;
    }

    alpha = spectrum[peak_bin - 1U];
    beta = spectrum[peak_bin];
    gamma = spectrum[peak_bin + 1U];

    delta = alpha - (2.0f * beta) + gamma;
    if (fabsf(delta) < 1e-12f)
    {
        return (float)peak_bin;
    }

    return (float)peak_bin + (0.5f * (alpha - gamma) / delta);
}

static float dsp_search_folded_harmonic(const float *spectrum,
                                        float fundamental_freq_hz,
                                        float sample_rate_hz,
                                        uint32_t harmonic_order,
                                        uint32_t fundamental_bin,
                                        float fundamental_amp,
                                        float min_ratio,
                                        bool *strong_collision_out)
{
    float folded_hz;
    uint32_t expected_bin;
    uint32_t start_bin;
    uint32_t end_bin;
    uint32_t best_bin = 0U;
    float best_value = 0.0f;
    float resolution;
    float dc_guard_hz;
    uint32_t bin;

    if ((spectrum == NULL) || (fundamental_amp <= 0.0f) || (harmonic_order < 2U))
    {
        if (strong_collision_out != NULL)
        {
            *strong_collision_out = false;
        }
        return 0.0f;
    }

    if (strong_collision_out != NULL)
    {
        *strong_collision_out = false;
    }

    folded_hz = dsp_fold_frequency(fundamental_freq_hz * (float)harmonic_order,
                                   sample_rate_hz);
    resolution = dsp_bin_resolution(sample_rate_hz);
    if (resolution <= 0.0f)
    {
        return 0.0f;
    }

    /*
     * A folded harmonic inside the guarded DC bins is removed with the mean or
     * too close to the fundamental exclusion region to be useful.
     */
    dc_guard_hz = (float)DSP_PEAK_GUARD_BINS * resolution;
    if (folded_hz < dc_guard_hz)
    {
        return 0.0f;
    }

    expected_bin = dsp_frequency_to_nearest_bin(folded_hz, sample_rate_hz);
    if (expected_bin >= DSP_SPECTRUM_SIZE)
    {
        return 0.0f;
    }

    start_bin = (expected_bin > DSP_HARMONIC_SEARCH_BINS) ?
                (expected_bin - DSP_HARMONIC_SEARCH_BINS) : 1U;
    end_bin = expected_bin + DSP_HARMONIC_SEARCH_BINS;
    if (end_bin >= DSP_SPECTRUM_SIZE)
    {
        end_bin = DSP_SPECTRUM_SIZE - 1U;
    }

    for (bin = start_bin; bin <= end_bin; ++bin)
    {
        if ((fundamental_bin > 0U) && (bin >= fundamental_bin - DSP_PEAK_GUARD_BINS) &&
            (bin <= fundamental_bin + DSP_PEAK_GUARD_BINS))
        {
            continue;
        }

        if (spectrum[bin] > best_value)
        {
            best_value = spectrum[bin];
            best_bin = bin;
        }
    }

    if ((best_bin == 0U) || (best_value < (min_ratio * fundamental_amp)))
    {
        return 0.0f;
    }

    /*
     * If an apparent harmonic is larger than half the fundamental, it is almost
     * certainly the other source signal rather than a true triangle harmonic.
     * This protects basic cases like 20 kHz + 60 kHz and 30 kHz + 90 kHz.
     */
    if (best_value > (0.5f * fundamental_amp))
    {
        if (strong_collision_out != NULL)
        {
            *strong_collision_out = true;
        }
        return 0.0f;
    }

    return best_value;
}

static DSP_WaveformType_t dsp_classify_waveform(float fundamental,
                                                float harmonic3,
                                                float harmonic5,
                                                bool allow_h5_only,
                                                float *ratio_out)
{
    float ratio3 = 0.0f;
    float ratio5 = 0.0f;

    if (fundamental > 0.0f)
    {
        ratio3 = harmonic3 / fundamental;
        ratio5 = harmonic5 / fundamental;
    }

    if (ratio_out != NULL)
    {
        *ratio_out = ratio3;
    }

    /*
     * A triangle wave has odd harmonics with amplitude roughly 1/n^2:
     * H3 ~= 11.1%, H5 ~= 4%. The thresholds are intentionally broad to absorb
     * window leakage and small amplitude differences.
     */
    if ((ratio3 >= 0.020f) && (ratio3 <= 0.28f))
    {
        return DSP_WAVE_TRIANGLE;
    }

    if (allow_h5_only && (ratio5 >= 0.025f) && (ratio5 <= 0.12f) && (ratio3 < 0.055f))
    {
        return DSP_WAVE_TRIANGLE;
    }

    if ((ratio3 > 0.28f) && (ratio3 <= 0.60f))
    {
        return DSP_WAVE_SQUARE;
    }

    if (ratio3 < 0.055f)
    {
        return DSP_WAVE_SINE;
    }

    return DSP_WAVE_UNKNOWN;
}

static void dsp_fill_component(SignalComponent_t *component,
                               const float *spectrum,
                               uint32_t peak_bin,
                               float sample_rate_hz)
{
    float precise_bin;
    float h3_amp;
    float h5_amp;
    float ratio = 0.0f;
    bool h3_collision = false;

    memset(component, 0, sizeof(*component));

    if ((peak_bin == 0U) || (peak_bin >= DSP_SPECTRUM_SIZE))
    {
        return;
    }

    precise_bin = dsp_parabolic_interp(spectrum, peak_bin);

    component->valid = true;
    component->peak_bin = (uint16_t)peak_bin;
    component->frequency_hz = (precise_bin * sample_rate_hz) / (float)DSP_FFT_SIZE;
    component->fundamental_amplitude = spectrum[peak_bin];

    /*
     * Search harmonics per component. Harmonics above Nyquist are folded back
     * to their sampled alias, so high-frequency triangles remain detectable at
     * 500 kSPS without changing CubeMX timer setup.
     */
    h3_amp = dsp_search_folded_harmonic(spectrum,
                                        component->frequency_hz,
                                        sample_rate_hz,
                                        3U,
                                        peak_bin,
                                        component->fundamental_amplitude,
                                        0.018f,
                                        &h3_collision);
    h5_amp = dsp_search_folded_harmonic(spectrum,
                                        component->frequency_hz,
                                        sample_rate_hz,
                                        5U,
                                        peak_bin,
                                        component->fundamental_amplitude,
                                        0.012f,
                                        NULL);

    component->third_harmonic_amplitude = h3_amp;
    component->fifth_harmonic_amplitude = h5_amp;
    component->wave_type = dsp_classify_waveform(component->fundamental_amplitude,
                                                 h3_amp,
                                                 h5_amp,
                                                 h3_collision,
                                                 &ratio);
    component->harmonic_ratio = ratio;
}

static bool dsp_near_frequency(float lhs_hz, float rhs_hz, float sample_rate_hz)
{
    return fabsf(lhs_hz - rhs_hz) <=
           ((float)DSP_HARMONIC_SEARCH_BINS * dsp_bin_resolution(sample_rate_hz));
}

static void dsp_correct_h5_alias_collision(SignalComponent_t *component_a,
                                           SignalComponent_t *component_b,
                                           float sample_rate_hz)
{
    SignalComponent_t *low;
    SignalComponent_t *high;
    float low_h5_hz;
    float high_h5_hz;
    float low_h5_ratio;

    if ((component_a == NULL) || (component_b == NULL) ||
        !component_a->valid || !component_b->valid)
    {
        return;
    }

    if (component_a->frequency_hz <= component_b->frequency_hz)
    {
        low = component_a;
        high = component_b;
    }
    else
    {
        low = component_b;
        high = component_a;
    }

    if ((low->wave_type != DSP_WAVE_TRIANGLE) ||
        (high->wave_type != DSP_WAVE_TRIANGLE) ||
        (low->third_harmonic_amplitude > 0.0f) ||
        (low->fundamental_amplitude <= 0.0f))
    {
        return;
    }

    low_h5_hz = dsp_fold_frequency(low->frequency_hz * 5.0f, sample_rate_hz);
    high_h5_hz = dsp_fold_frequency(high->frequency_hz * 5.0f, sample_rate_hz);
    if (!dsp_near_frequency(low_h5_hz, high_h5_hz, sample_rate_hz))
    {
        return;
    }

    low_h5_ratio = low->fifth_harmonic_amplitude / low->fundamental_amplitude;
    if (low_h5_ratio < DSP_H5_COLLISION_TRI_RATIO)
    {
        low->wave_type = DSP_WAVE_SINE;
        low->harmonic_ratio = 0.0f;
    }
}

static void Analyze_Signal(float *fft_output, float sample_rate_hz)
{
    uint32_t start_bin;
    uint32_t end_bin;
    uint32_t primary_bin;
    uint32_t secondary_bin;

    if (fft_output == NULL)
    {
        g_last_analysis.valid = false;
        return;
    }

    start_bin = dsp_frequency_to_bin(DSP_MIN_ANALYSIS_FREQ_HZ, sample_rate_hz);
    end_bin = dsp_frequency_to_bin(DSP_MAX_ANALYSIS_FREQ_HZ, sample_rate_hz);

    if (end_bin >= DSP_SPECTRUM_SIZE)
    {
        end_bin = DSP_SPECTRUM_SIZE - 1U;
    }

    if (start_bin >= end_bin)
    {
        g_last_analysis.valid = false;
        return;
    }

    primary_bin = dsp_find_peak(fft_output, start_bin, end_bin, -1);
    secondary_bin = dsp_find_peak(fft_output, start_bin, end_bin, (int32_t)primary_bin);

    dsp_fill_component(&g_last_analysis.primary, fft_output, primary_bin, sample_rate_hz);
    if ((g_last_analysis.primary.fundamental_amplitude > 0.0f) &&
        (secondary_bin > 0U) &&
        (fft_output[secondary_bin] >=
         (DSP_SECONDARY_MIN_RATIO * g_last_analysis.primary.fundamental_amplitude)))
    {
        dsp_fill_component(&g_last_analysis.secondary, fft_output, secondary_bin, sample_rate_hz);
    }
    else
    {
        memset(&g_last_analysis.secondary, 0, sizeof(g_last_analysis.secondary));
    }

    dsp_correct_h5_alias_collision(&g_last_analysis.primary,
                                   &g_last_analysis.secondary,
                                   sample_rate_hz);

    g_last_analysis.valid = g_last_analysis.primary.valid || g_last_analysis.secondary.valid;
}

void DSP_Process_Init(void)
{
    memset(&g_last_analysis, 0, sizeof(g_last_analysis));
    memset(g_fft_buffer, 0, sizeof(g_fft_buffer));
    memset(g_spectrum, 0, sizeof(g_spectrum));
}

HAL_StatusTypeDef DSP_ProcessSamples(const int16_t *samples, uint32_t sample_count, float sample_rate_hz)
{
    float mean = 0.0f;
    float spectrum_scale = 2.0f / ((float)DSP_FFT_SIZE * DSP_HANN_COHERENT_GAIN);
    uint32_t index;

    if ((samples == NULL) || (sample_count != DSP_FFT_SIZE) || (sample_rate_hz <= 0.0f))
    {
        return HAL_ERROR;
    }

    memset(&g_last_analysis, 0, sizeof(g_last_analysis));
    g_last_analysis.sample_count = sample_count;
    g_last_analysis.sample_rate_hz = sample_rate_hz;
    g_last_analysis.spectrum = g_spectrum;
    g_last_analysis.spectrum_bins = DSP_SPECTRUM_SIZE;

    for (index = 0U; index < sample_count; ++index)
    {
        mean += (float)samples[index];
    }
    mean /= (float)sample_count;

    for (index = 0U; index < sample_count; ++index)
    {
        float sample = ((float)samples[index] - mean) * dsp_hann_window(index);

        g_fft_buffer[2U * index] = sample;
        g_fft_buffer[(2U * index) + 1U] = 0.0f;
    }

    arm_cfft_f32(&arm_cfft_sR_f32_len2048, g_fft_buffer, 0U, 1U);

    for (index = 0U; index < DSP_SPECTRUM_SIZE; ++index)
    {
        float real = g_fft_buffer[2U * index];
        float imag = g_fft_buffer[(2U * index) + 1U];
        g_spectrum[index] = spectrum_scale * sqrtf((real * real) + (imag * imag));
    }

    Analyze_Signal(g_spectrum, sample_rate_hz);
    return HAL_OK;
}

const SignalAnalysisResult_t *DSP_GetLastAnalysis(void)
{
    return &g_last_analysis;
}

const float *DSP_GetSpectrum(void)
{
    return g_spectrum;
}

const char *DSP_WaveTypeToString(DSP_WaveformType_t wave_type)
{
    switch (wave_type)
    {
    case DSP_WAVE_SINE:
        return "SINE";
    case DSP_WAVE_TRIANGLE:
        return "TRI";
    case DSP_WAVE_SQUARE:
        return "SQUARE";
    case DSP_WAVE_UNKNOWN:
    default:
        return "UNKNOWN";
    }
}
