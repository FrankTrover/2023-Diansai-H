#include "lcd_ui.h"

#include <stdio.h>

#include "lcd.h"

#define LCD_UI_TEXT_COLOR     WHITE
#define LCD_UI_BG_COLOR       BLACK
#define LCD_UI_ACCENT_COLOR   GBLUE
#define LCD_UI_WARN_COLOR     YELLOW
#define LCD_UI_ERROR_COLOR    RED

static void lcd_ui_draw_header(const char *title)
{
    LCD_Fill(0, 0, LCD_W, LCD_H, LCD_UI_BG_COLOR);
    LCD_DrawRectangle(0, 0, LCD_W - 1U, 18U, LCD_UI_ACCENT_COLOR);
    LCD_ShowString(4U, 2U, (const u8 *)title, LCD_UI_TEXT_COLOR, LCD_UI_BG_COLOR, 16U, 0U);
}

static void lcd_ui_draw_text_line(uint16_t x, uint16_t y, const char *text, uint16_t color)
{
    LCD_ShowString(x, y, (const u8 *)text, color, LCD_UI_BG_COLOR, 16U, 0U);
}

static const char *lcd_ui_wave_short(DSP_WaveformType_t wave_type)
{
    switch (wave_type)
    {
    case DSP_WAVE_SINE:
        return "SIN";
    case DSP_WAVE_TRIANGLE:
        return "TRI";
    case DSP_WAVE_SQUARE:
        return "SQR";
    case DSP_WAVE_UNKNOWN:
    default:
        return "UNK";
    }
}

static void lcd_ui_format_khz(char *line,
                              size_t line_size,
                              const char *prefix,
                              uint32_t frequency_hz,
                              const char *wave_text)
{
    uint32_t khz10 = (frequency_hz + 50U) / 100U;

    if (wave_text != NULL)
    {
        snprintf(line,
                 line_size,
                 "%s:%3lu.%1lu kHz %s",
                 prefix,
                 (unsigned long)(khz10 / 10U),
                 (unsigned long)(khz10 % 10U),
                 wave_text);
    }
    else
    {
        snprintf(line,
                 line_size,
                 "%s:%3lu.%1lu kHz",
                 prefix,
                 (unsigned long)(khz10 / 10U),
                 (unsigned long)(khz10 % 10U));
    }
}

static void lcd_ui_format_khz_short(char *line,
                                    size_t line_size,
                                    const char *prefix,
                                    uint32_t frequency_hz,
                                    const char *wave_text)
{
    uint32_t khz10 = (frequency_hz + 50U) / 100U;

    snprintf(line,
             line_size,
             "%s:%3lu.%1luk %s",
             prefix,
             (unsigned long)(khz10 / 10U),
             (unsigned long)(khz10 % 10U),
             wave_text);
}

static void lcd_ui_draw_spectrum(const SignalAnalysisResult_t *result)
{
    uint32_t bar;
    uint32_t bins_per_bar;
    float max_value = 0.0f;
    const uint32_t bar_count = 32U;
    const uint16_t chart_x0 = 4U;
    const uint16_t chart_y0 = 154U;
    const uint16_t chart_h = 50U;

    if ((result == NULL) || (result->spectrum == NULL) || (result->spectrum_bins == 0U))
    {
        return;
    }

    bins_per_bar = result->spectrum_bins / bar_count;
    if (bins_per_bar == 0U)
    {
        bins_per_bar = 1U;
    }

    for (bar = 0U; bar < result->spectrum_bins; ++bar)
    {
        if (result->spectrum[bar] > max_value)
        {
            max_value = result->spectrum[bar];
        }
    }

    if (max_value <= 0.0f)
    {
        max_value = 1.0f;
    }

    LCD_DrawRectangle(chart_x0 - 2U, chart_y0 - chart_h - 2U, LCD_W - 2U, chart_y0 + 2U, LCD_UI_ACCENT_COLOR);

    for (bar = 0U; bar < bar_count; ++bar)
    {
        uint32_t bin_start = bar * bins_per_bar;
        uint32_t bin_end = bin_start + bins_per_bar;
        uint32_t bin;
        float bar_value = 0.0f;
        uint16_t bar_height;
        uint16_t x = chart_x0 + (uint16_t)(bar * 4U);

        if (bin_end > result->spectrum_bins)
        {
            bin_end = result->spectrum_bins;
        }

        for (bin = bin_start; bin < bin_end; ++bin)
        {
            if (result->spectrum[bin] > bar_value)
            {
                bar_value = result->spectrum[bin];
            }
        }

        bar_height = (uint16_t)((bar_value / max_value) * (float)chart_h);
        if (bar_height > chart_h)
        {
            bar_height = chart_h;
        }

        LCD_DrawLine(x, chart_y0, x, (uint16_t)(chart_y0 - bar_height), GREEN);
        LCD_DrawLine((uint16_t)(x + 1U), chart_y0, (uint16_t)(x + 1U), (uint16_t)(chart_y0 - bar_height), GREEN);
    }
}

void LCD_UI_Init(void)
{
    LCD_Init();
}

void LCD_UI_ShowIdle(void)
{
    lcd_ui_draw_header("Signal Splitter");
    lcd_ui_draw_text_line(4U, 28U, "Ready", LCD_UI_TEXT_COLOR);
    lcd_ui_draw_text_line(4U, 48U, "Auto scanning...", LCD_UI_TEXT_COLOR);
}

void LCD_UI_ShowAcquiring(void)
{
    lcd_ui_draw_header("Signal Splitter");
    lcd_ui_draw_text_line(4U, 28U, "Sampling AD7616...", LCD_UI_WARN_COLOR);
    lcd_ui_draw_text_line(4U, 48U, "FFT frame: 2048 pts", LCD_UI_TEXT_COLOR);
}

void LCD_UI_ShowAnalysis(const SignalAnalysisResult_t *result, float output_vpp)
{
    char line[40];

    lcd_ui_draw_header("Analysis Result");

    if ((result == NULL) || !result->valid)
    {
        lcd_ui_draw_text_line(4U, 28U, "No valid peak", LCD_UI_WARN_COLOR);
        return;
    }

    lcd_ui_format_khz(line, sizeof(line), "P1", (uint32_t)(result->primary.frequency_hz + 0.5f), NULL);
    lcd_ui_draw_text_line(4U, 24U, line, LCD_UI_TEXT_COLOR);

    snprintf(line, sizeof(line), "W1:%s", DSP_WaveTypeToString(result->primary.wave_type));
    lcd_ui_draw_text_line(4U, 40U, line, LCD_UI_TEXT_COLOR);

    if (result->secondary.valid)
    {
        lcd_ui_format_khz(line, sizeof(line), "P2", (uint32_t)(result->secondary.frequency_hz + 0.5f), NULL);
        lcd_ui_draw_text_line(4U, 56U, line, LCD_UI_TEXT_COLOR);

        snprintf(line, sizeof(line), "W2:%s", DSP_WaveTypeToString(result->secondary.wave_type));
        lcd_ui_draw_text_line(4U, 72U, line, LCD_UI_TEXT_COLOR);
    }
    else
    {
        lcd_ui_draw_text_line(4U, 56U, "P2: --", LCD_UI_TEXT_COLOR);
        lcd_ui_draw_text_line(4U, 72U, "W2: --", LCD_UI_TEXT_COLOR);
    }

    {
        uint32_t vpp100 = (uint32_t)((output_vpp * 100.0f) + 0.5f);
        snprintf(line,
                 sizeof(line),
                 "OUT:%lu.%02lu Vpp",
                 (unsigned long)(vpp100 / 100U),
                 (unsigned long)(vpp100 % 100U));
    }
    lcd_ui_draw_text_line(4U, 90U, line, LCD_UI_TEXT_COLOR);

    lcd_ui_draw_spectrum(result);
}

void LCD_UI_ShowSplitResult(const LCD_SplitResult_t *result)
{
    char line[40];

    lcd_ui_draw_header("Split Result");

    if ((result == NULL) || !result->valid)
    {
        lcd_ui_draw_text_line(4U, 28U, "No valid split", LCD_UI_WARN_COLOR);
        return;
    }

    /* 输入端：检测到的信号 */
    lcd_ui_draw_text_line(4U, 22U, "[INPUT]", LCD_UI_ACCENT_COLOR);

    lcd_ui_format_khz(line,
                      sizeof(line),
                      "A",
                      result->input_a.freq_hz,
                      DSP_WaveTypeToString(result->input_a.wave_type));
    lcd_ui_draw_text_line(4U, 38U, line, LCD_UI_TEXT_COLOR);

    lcd_ui_format_khz(line,
                      sizeof(line),
                      "B",
                      result->input_b.freq_hz,
                      DSP_WaveTypeToString(result->input_b.wave_type));
    lcd_ui_draw_text_line(4U, 54U, line, LCD_UI_TEXT_COLOR);

    /* 输出端：DDS 实际输出 */
    lcd_ui_draw_text_line(4U, 74U, "[DDS OUT]", LCD_UI_ACCENT_COLOR);

    lcd_ui_format_khz_short(line,
                            sizeof(line),
                            "D3",
                            result->dds_a.freq_hz,
                            lcd_ui_wave_short(result->dds_a.wave_type));
    lcd_ui_draw_text_line(4U, 90U, line, GREEN);

    lcd_ui_format_khz_short(line,
                            sizeof(line),
                            "D4",
                            result->dds_b.freq_hz,
                            lcd_ui_wave_short(result->dds_b.wave_type));
    lcd_ui_draw_text_line(4U, 106U, line, GREEN);
}

void LCD_UI_ShowError(const char *message)
{
    lcd_ui_draw_header("System Error");
    lcd_ui_draw_text_line(4U, 28U, (message != NULL) ? message : "Unknown error", LCD_UI_ERROR_COLOR);
}
