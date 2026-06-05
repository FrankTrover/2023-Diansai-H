#include "ad7616.h"

#include <stddef.h>

#include "delay.h"

static uint8_t g_range = Range_10_V;
static uint8_t g_mode = HARDWARE_MODE;

static GPIO_TypeDef *const g_port_list[16] = {
    AD7616_D0_GPIO_Port, AD7616_D1_GPIO_Port, AD7616_D2_GPIO_Port, AD7616_D3_GPIO_Port,
    AD7616_D4_GPIO_Port, AD7616_D5_GPIO_Port, AD7616_D6_GPIO_Port, AD7616_D7_GPIO_Port,
    AD7616_D8_GPIO_Port, AD7616_D9_GPIO_Port, AD7616_D10_GPIO_Port, AD7616_D11_GPIO_Port,
    AD7616_D12_GPIO_Port, AD7616_D13_GPIO_Port, AD7616_D14_GPIO_Port, AD7616_D15_GPIO_Port
};

static const uint16_t g_pin_list[16] = {
    AD7616_D0_Pin, AD7616_D1_Pin, AD7616_D2_Pin, AD7616_D3_Pin,
    AD7616_D4_Pin, AD7616_D5_Pin, AD7616_D6_Pin, AD7616_D7_Pin,
    AD7616_D8_Pin, AD7616_D9_Pin, AD7616_D10_Pin, AD7616_D11_Pin,
    AD7616_D12_Pin, AD7616_D13_Pin, AD7616_D14_Pin, AD7616_D15_Pin
};

static uint8_t ad7616_normalize_range(uint8_t range)
{
    switch (range)
    {
    case Range_2_5_V:
    case Range_5_V:
    case Range_10_V:
        return range;
    default:
        return Range_10_V;
    }
}

static void AD7616_Enable_GPIO_Clock(GPIO_TypeDef *port)
{
    if (port == GPIOA)
    {
        __HAL_RCC_GPIOA_CLK_ENABLE();
    }
    else if (port == GPIOB)
    {
        __HAL_RCC_GPIOB_CLK_ENABLE();
    }
    else if (port == GPIOC)
    {
        __HAL_RCC_GPIOC_CLK_ENABLE();
    }
    else if (port == GPIOD)
    {
        __HAL_RCC_GPIOD_CLK_ENABLE();
    }
    else if (port == GPIOE)
    {
        __HAL_RCC_GPIOE_CLK_ENABLE();
    }
    else if (port == GPIOH)
    {
        __HAL_RCC_GPIOH_CLK_ENABLE();
    }
}

static void AD7616_Enable_All_GPIO_Clocks(void)
{
    uint8_t i;

    for (i = 0U; i < 16U; ++i)
    {
        AD7616_Enable_GPIO_Clock(g_port_list[i]);
    }

    AD7616_Enable_GPIO_Clock(AD7616_CS_GPIO_Port);
    AD7616_Enable_GPIO_Clock(AD7616_RD_GPIO_Port);
    AD7616_Enable_GPIO_Clock(AD7616_WR_GPIO_Port);
    /* CONV (PA15) 时钟由 TIM2 驱动，无需在此使能 */
    AD7616_Enable_GPIO_Clock(AD7616_RST_GPIO_Port);
    AD7616_Enable_GPIO_Clock(AD7616_BUSY_GPIO_Port);
    AD7616_Enable_GPIO_Clock(AD7616_RNG0_GPIO_Port);
    AD7616_Enable_GPIO_Clock(AD7616_RNG1_GPIO_Port);
    AD7616_Enable_GPIO_Clock(AD7616_CHS0_GPIO_Port);
    AD7616_Enable_GPIO_Clock(AD7616_CHS1_GPIO_Port);
    AD7616_Enable_GPIO_Clock(AD7616_CHS2_GPIO_Port);
    AD7616_Enable_GPIO_Clock(AD7616_SER_GPIO_Port);
}

static GPIO_PinState ad7616_read_bit(GPIO_TypeDef *port, uint16_t pin)
{
    return HAL_GPIO_ReadPin(port, pin);
}

static void ad7616_write_bit(GPIO_TypeDef *port, uint16_t pin, GPIO_PinState level)
{
    HAL_GPIO_WritePin(port, pin, level);
}

static void AD7616_Init_Control_GPIOs(void)
{
    GPIO_InitTypeDef init = {0};

    init.Mode = GPIO_MODE_OUTPUT_PP;
    init.Pull = GPIO_NOPULL;
    init.Speed = GPIO_SPEED_FREQ_VERY_HIGH;

    init.Pin = AD7616_CS_Pin;
    HAL_GPIO_Init(AD7616_CS_GPIO_Port, &init);
    init.Pin = AD7616_RD_Pin;
    HAL_GPIO_Init(AD7616_RD_GPIO_Port, &init);
    init.Pin = AD7616_WR_Pin;
    HAL_GPIO_Init(AD7616_WR_GPIO_Port, &init);
    /* CONV (PA15) 已由 TIM2 PWM 控制，不再作为 GPIO 初始化 */
    init.Pin = AD7616_RST_Pin;
    HAL_GPIO_Init(AD7616_RST_GPIO_Port, &init);
    init.Pin = AD7616_RNG0_Pin;
    HAL_GPIO_Init(AD7616_RNG0_GPIO_Port, &init);
    init.Pin = AD7616_RNG1_Pin;
    HAL_GPIO_Init(AD7616_RNG1_GPIO_Port, &init);
    init.Pin = AD7616_CHS0_Pin;
    HAL_GPIO_Init(AD7616_CHS0_GPIO_Port, &init);
    init.Pin = AD7616_CHS1_Pin;
    HAL_GPIO_Init(AD7616_CHS1_GPIO_Port, &init);
    init.Pin = AD7616_CHS2_Pin;
    HAL_GPIO_Init(AD7616_CHS2_GPIO_Port, &init);
    init.Pin = AD7616_SER_Pin;
    HAL_GPIO_Init(AD7616_SER_GPIO_Port, &init);

    init.Mode = GPIO_MODE_INPUT;
    init.Pull = GPIO_NOPULL;
    init.Pin = AD7616_BUSY_Pin;
    HAL_GPIO_Init(AD7616_BUSY_GPIO_Port, &init);
}

static void AD7616_Set_Control_Idle(void)
{
    ad7616_write_bit(AD7616_CS_GPIO_Port, AD7616_CS_Pin, GPIO_PIN_SET);
    ad7616_write_bit(AD7616_RD_GPIO_Port, AD7616_RD_Pin, GPIO_PIN_SET);
    ad7616_write_bit(AD7616_WR_GPIO_Port, AD7616_WR_Pin, GPIO_PIN_SET);
    /* CONV (PA15) 由 TIM2 PWM 控制，不在这里置高 */
    ad7616_write_bit(AD7616_RST_GPIO_Port, AD7616_RST_Pin, GPIO_PIN_SET);
}

static HAL_StatusTypeDef AD7616_Wait_Busy_Ready(uint32_t timeout_us)
{
    uint32_t start_tick = DWT->CYCCNT;
    uint32_t timeout_ticks = timeout_us * (HAL_RCC_GetSysClockFreq() / 1000000U);

    while (ad7616_read_bit(AD7616_BUSY_GPIO_Port, AD7616_BUSY_Pin) == GPIO_PIN_SET)
    {
        if ((DWT->CYCCNT - start_tick) >= timeout_ticks)
        {
            return HAL_TIMEOUT;
        }
    }

    return HAL_OK;
}

static void AD7616_Working_Mode(uint8_t mode)
{
    ad7616_write_bit(AD7616_SER_GPIO_Port,
                     AD7616_SER_Pin,
                     (mode == HARDWARE_MODE) ? GPIO_PIN_RESET : GPIO_PIN_SET);
}

static void AD7616_Set_Pin_Output(void)
{
    GPIO_InitTypeDef init = {0};
    uint8_t i;

    init.Mode = GPIO_MODE_OUTPUT_PP;
    init.Pull = GPIO_NOPULL;
    init.Speed = GPIO_SPEED_FREQ_HIGH;

    for (i = 0U; i < 16U; ++i)
    {
        init.Pin = g_pin_list[i];
        HAL_GPIO_Init(g_port_list[i], &init);
    }
}

static void AD7616_Set_Pin_Input(void)
{
    GPIO_InitTypeDef init = {0};
    uint8_t i;

    init.Mode = GPIO_MODE_INPUT;
    init.Pull = GPIO_NOPULL;

    for (i = 0U; i < 16U; ++i)
    {
        init.Pin = g_pin_list[i];
        HAL_GPIO_Init(g_port_list[i], &init);
    }
}

static uint16_t AD7616_Read_Data_16Bit(void)
{
    /*
     * 所有数据引脚 D0-D15 均在 GPIOD（PD0-PD15），直接读 IDR 寄存器。
     * 原实现：16 次 HAL_GPIO_ReadPin 循环，约 250 周期/采样。
     * 新实现：1 次寄存器读取，约 4 周期/采样，速度提升 ~60 倍。
     */
    return (uint16_t)(GPIOD->IDR & 0xFFFFU);
}

static HAL_StatusTypeDef AD7616_Read_Current_Frame(int16_t *data_A, int16_t *data_B)
{
    if ((data_A == NULL) || (data_B == NULL))
    {
        return HAL_ERROR;
    }

    ad7616_write_bit(AD7616_CS_GPIO_Port, AD7616_CS_Pin, GPIO_PIN_RESET);

    if (g_mode == HARDWARE_MODE)
    {
        /*
         * 高速并行读取：去掉 delay_us(1)，用 __NOP() 做亚微秒级延时。
         * AD7616 t12 (CS↓ 到数据有效) = 0ns，t16 (RD↓ 到数据有效) = 25ns max。
         * 168MHz 下 1 个 NOP ≈ 6ns，4 个 NOP ≈ 24ns，满足 t16 要求。
         */
        ad7616_write_bit(AD7616_RD_GPIO_Port, AD7616_RD_Pin, GPIO_PIN_RESET);
        __NOP(); __NOP(); __NOP(); __NOP();
        *data_A = (int16_t)AD7616_Read_Data_16Bit();
        ad7616_write_bit(AD7616_RD_GPIO_Port, AD7616_RD_Pin, GPIO_PIN_SET);
        __NOP(); __NOP();

        ad7616_write_bit(AD7616_RD_GPIO_Port, AD7616_RD_Pin, GPIO_PIN_RESET);
        __NOP(); __NOP(); __NOP(); __NOP();
        *data_B = (int16_t)AD7616_Read_Data_16Bit();
        ad7616_write_bit(AD7616_RD_GPIO_Port, AD7616_RD_Pin, GPIO_PIN_SET);
    }
    else
    {
        AD7616_Read_Serial(data_A, data_B);
    }

    ad7616_write_bit(AD7616_RD_GPIO_Port, AD7616_RD_Pin, GPIO_PIN_SET);
    ad7616_write_bit(AD7616_CS_GPIO_Port, AD7616_CS_Pin, GPIO_PIN_SET);
    return HAL_OK;
}

GPIO_PinState AD7616_IsBusy(void)
{
    return ad7616_read_bit(AD7616_BUSY_GPIO_Port, AD7616_BUSY_Pin);
}

void AD7616_Serial_GPIO_Init(void)
{
    GPIO_InitTypeDef init = {0};

    AD7616_Set_Pin_Output();

    init.Mode = GPIO_MODE_INPUT;
    init.Pull = GPIO_NOPULL;

    init.Pin = AD7616_SDOB_Pin;
    HAL_GPIO_Init(AD7616_SDOB_GPIO_Port, &init);

    init.Pin = AD7616_SDOA_Pin;
    HAL_GPIO_Init(AD7616_SDOA_GPIO_Port, &init);
}

void AD7616_Parallel_GPIO_Init(void)
{
    AD7616_Set_Pin_Input();
}

void AD7616_Reset(void)
{
    ad7616_write_bit(AD7616_RST_GPIO_Port, AD7616_RST_Pin, GPIO_PIN_RESET);
    delay_ms(1U);
    ad7616_write_bit(AD7616_RST_GPIO_Port, AD7616_RST_Pin, GPIO_PIN_SET);
    delay_ms(20U);
}

/**
 * @brief [已废弃] 软件触发 AD7616 转换。
 * @note  PA15 已配置为 TIM2_CH1 PWM 自动产生 CONV 脉冲，
 *        此函数不再被主程序调用。保留仅为向后兼容。
 */
void AD7616_Conversion(void)
{
    /* Deprecated no-op: PA15 is driven by TIM2. */
    /* WARNING: PA15 当前为 TIM2 AF 模式，直接写 GPIO 无效 */
    (void)AD7616_CONV_GPIO_Port;
    (void)AD7616_CONV_Pin;
}

void AD7616_Parallel_Set_voltage(uint8_t range)
{
    g_range = ad7616_normalize_range(range);

    switch (g_range)
    {
    case Range_2_5_V:
        ad7616_write_bit(AD7616_RNG0_GPIO_Port, AD7616_RNG0_Pin, GPIO_PIN_SET);
        ad7616_write_bit(AD7616_RNG1_GPIO_Port, AD7616_RNG1_Pin, GPIO_PIN_RESET);
        break;
    case Range_5_V:
        ad7616_write_bit(AD7616_RNG0_GPIO_Port, AD7616_RNG0_Pin, GPIO_PIN_RESET);
        ad7616_write_bit(AD7616_RNG1_GPIO_Port, AD7616_RNG1_Pin, GPIO_PIN_SET);
        break;
    case Range_10_V:
    default:
        ad7616_write_bit(AD7616_RNG0_GPIO_Port, AD7616_RNG0_Pin, GPIO_PIN_SET);
        ad7616_write_bit(AD7616_RNG1_GPIO_Port, AD7616_RNG1_Pin, GPIO_PIN_SET);
        break;
    }
}

void AD7616_Read_Serial(int16_t *data_A, int16_t *data_B)
{
    uint16_t shift = 0x8000U;
    uint16_t i;
    int16_t input_a = 0;
    int16_t input_b = 0;

    if ((data_A == NULL) || (data_B == NULL))
    {
        return;
    }

    for (i = 0U; i < 16U; ++i)
    {
        ad7616_write_bit(AD7616_RD_GPIO_Port, AD7616_RD_Pin, GPIO_PIN_RESET);
        delay_us(1U);

        if (ad7616_read_bit(AD7616_SDOA_GPIO_Port, AD7616_SDOA_Pin) == GPIO_PIN_SET)
        {
            input_a |= (int16_t)shift;
        }

        if (ad7616_read_bit(AD7616_SDOB_GPIO_Port, AD7616_SDOB_Pin) == GPIO_PIN_SET)
        {
            input_b |= (int16_t)shift;
        }

        shift >>= 1U;
        ad7616_write_bit(AD7616_RD_GPIO_Port, AD7616_RD_Pin, GPIO_PIN_SET);
        delay_us(1U);
    }

    *data_A = input_a;
    *data_B = input_b;
}

void AD7616_Read_Data(int16_t *data_A, int16_t *data_B)
{
    (void)AD7616_Read_Data_Timeout(data_A, data_B, AD7616_BUSY_TIMEOUT_US);
}

HAL_StatusTypeDef AD7616_Read_Data_NonBlocking(int16_t *data_A, int16_t *data_B)
{
    if ((data_A == NULL) || (data_B == NULL))
    {
        return HAL_ERROR;
    }

    if (AD7616_IsBusy() == GPIO_PIN_SET)
    {
        return HAL_BUSY;
    }

    return AD7616_Read_Current_Frame(data_A, data_B);
}

HAL_StatusTypeDef AD7616_Read_Data_Timeout(int16_t *data_A, int16_t *data_B, uint32_t timeout_us)
{
    if ((data_A == NULL) || (data_B == NULL))
    {
        return HAL_ERROR;
    }

    if (AD7616_Wait_Busy_Ready(timeout_us) != HAL_OK)
    {
        return HAL_TIMEOUT;
    }

    return AD7616_Read_Current_Frame(data_A, data_B);
}

void AD7616_Parallel_Channel_Select(uint8_t channel)
{
    channel &= 0x07U;

    ad7616_write_bit(AD7616_CHS0_GPIO_Port, AD7616_CHS0_Pin, ((channel & 0x01U) != 0U) ? GPIO_PIN_SET : GPIO_PIN_RESET);
    ad7616_write_bit(AD7616_CHS1_GPIO_Port, AD7616_CHS1_Pin, ((channel & 0x02U) != 0U) ? GPIO_PIN_SET : GPIO_PIN_RESET);
    ad7616_write_bit(AD7616_CHS2_GPIO_Port, AD7616_CHS2_Pin, ((channel & 0x04U) != 0U) ? GPIO_PIN_SET : GPIO_PIN_RESET);
}

void AD7616_Init(uint8_t mode)
{
    if (mode != SOFTWARE_MODE)
    {
        mode = HARDWARE_MODE;
    }

    g_mode = mode;
    AD7616_Enable_All_GPIO_Clocks();
    AD7616_Init_Control_GPIOs();

    if (mode == HARDWARE_MODE)
    {
        AD7616_Parallel_GPIO_Init();
    }
    else
    {
        AD7616_Serial_GPIO_Init();
    }

    AD7616_Set_Control_Idle();
    AD7616_Working_Mode(mode);
}

void AD7616_Write_Serial(uint16_t data)
{
    uint16_t shift = 0x8000U;
    uint16_t i;

    for (i = 0U; i < 16U; ++i)
    {
        ad7616_write_bit(AD7616_SDI_GPIO_Port, AD7616_SDI_Pin, ((data & shift) != 0U) ? GPIO_PIN_SET : GPIO_PIN_RESET);
        ad7616_write_bit(AD7616_RD_GPIO_Port, AD7616_RD_Pin, GPIO_PIN_RESET);
        delay_us(1U);
        ad7616_write_bit(AD7616_RD_GPIO_Port, AD7616_RD_Pin, GPIO_PIN_SET);
        delay_us(1U);
        shift >>= 1U;
    }
}

void AD7616_Serial_Write_Rang(uint8_t address, uint8_t data)
{
    uint16_t write_data = (uint16_t)(((uint16_t)address << 9) | 0x8000U | data);

    ad7616_write_bit(AD7616_CS_GPIO_Port, AD7616_CS_Pin, GPIO_PIN_RESET);
    AD7616_Write_Serial(write_data);
    ad7616_write_bit(AD7616_CS_GPIO_Port, AD7616_CS_Pin, GPIO_PIN_SET);
}

void AD7616_Serial_Set_voltage(uint8_t range)
{
    uint8_t normalized_range = ad7616_normalize_range(range);
    uint8_t range_data = (uint8_t)((normalized_range << 6) | (normalized_range << 4) | (normalized_range << 2) | normalized_range);

    g_range = normalized_range;
    AD7616_Serial_Write_Rang(Input_Range_Register_A1, range_data);
    AD7616_Serial_Write_Rang(Input_Range_Register_A2, range_data);
    AD7616_Serial_Write_Rang(Input_Range_Register_B1, range_data);
    AD7616_Serial_Write_Rang(Input_Range_Register_B2, range_data);
}

void AD7616_Seria_Channel(uint8_t channel)
{
    uint8_t channel_data;

    channel &= 0x07U;
    channel_data = (uint8_t)((channel << 4) | channel);
    AD7616_Serial_Write_Rang(Channel_Register, channel_data);
}

void AD7616_Serial_Output_Format(uint8_t format)
{
    ad7616_write_bit(AD7616_SER1W_GPIO_Port,
                     AD7616_SER1W_Pin,
                     (format == Serial_Line_2_Output) ? GPIO_PIN_SET : GPIO_PIN_RESET);
}
