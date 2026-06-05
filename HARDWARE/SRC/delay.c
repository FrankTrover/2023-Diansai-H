#include "delay.h"

static uint32_t fac_us;
static uint32_t sys_clk;

static void dwt_init(void)
{
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
    DWT->CTRL = 0U;
    DWT->CYCCNT = 0U;
    DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
}

void delay_init(uint32_t sysclk_freq)
{
    sys_clk = sysclk_freq;
    fac_us = sysclk_freq / 1000000U;
    dwt_init();
}

void delay_us(uint32_t us)
{
    uint32_t ticks_start = DWT->CYCCNT;
    uint32_t delay_ticks = us * fac_us;

    while ((DWT->CYCCNT - ticks_start) < delay_ticks)
    {
    }
}

void delay_ms(uint32_t ms)
{
    if (ms == 0U)
    {
        return;
    }

    while (ms > 0U)
    {
        uint32_t chunk_ms = (ms > 4000000U) ? 4000000U : ms;
        delay_us(chunk_ms * 1000U);
        ms -= chunk_ms;
    }
}

void delay_ns(uint32_t ns)
{
    uint32_t cycles = (ns * sys_clk) / 1000000000U;
    uint32_t start;

    if (cycles == 0U)
    {
        cycles = 1U;
    }

    start = DWT->CYCCNT;
    while ((DWT->CYCCNT - start) < cycles)
    {
    }
}