#ifndef __AD7616_H__
#define __AD7616_H__

#include <stdint.h>

#include "main.h"
#include "ad7616_port.h"

#define AD7616_SDI_Pin AD7616_D10_Pin
#define AD7616_SDI_GPIO_Port AD7616_D10_GPIO_Port
#define AD7616_SDOB_Pin AD7616_D11_Pin
#define AD7616_SDOB_GPIO_Port AD7616_D11_GPIO_Port
#define AD7616_SDOA_Pin AD7616_D12_Pin
#define AD7616_SDOA_GPIO_Port AD7616_D12_GPIO_Port
#define AD7616_SER1W_Pin AD7616_D4_Pin
#define AD7616_SER1W_GPIO_Port AD7616_D4_GPIO_Port
#define AD7616_SER_Pin AD7616_S_P_Pin
#define AD7616_SER_GPIO_Port AD7616_S_P_GPIO_Port

#define HARDWARE_MODE 0U
#define SOFTWARE_MODE 1U

enum AD7616_Register_Address
{
    Config_Register = 0x02,
    Channel_Register = 0x03,
    Input_Range_Register_A1 = 0x04,
    Input_Range_Register_A2 = 0x05,
    Input_Range_Register_B1 = 0x06,
    Input_Range_Register_B2 = 0x07,
};

enum AD7616_Range
{
    Range_10_V = 0x00,
    Range_2_5_V = 0x01,
    Range_5_V = 0x02,
};

enum AD7616_Serial_Output_Format
{
    Serial_Line_1_Output = 0x00,
    Serial_Line_2_Output = 0x01,
};

#define AD7616_BUSY_TIMEOUT_US 5000U

GPIO_PinState AD7616_IsBusy(void);
void AD7616_Parallel_GPIO_Init(void);
void AD7616_Serial_GPIO_Init(void);
void AD7616_Init(uint8_t mode);
void AD7616_Reset(void);
void AD7616_Conversion(void);
void AD7616_Parallel_Set_voltage(uint8_t range);
void AD7616_Read_Data(int16_t *data_A, int16_t *data_B);
HAL_StatusTypeDef AD7616_Read_Data_NonBlocking(int16_t *data_A, int16_t *data_B);
HAL_StatusTypeDef AD7616_Read_Data_Timeout(int16_t *data_A, int16_t *data_B, uint32_t timeout_us);
void AD7616_Parallel_Channel_Select(uint8_t channel);
void AD7616_Write_Serial(uint16_t data);
void AD7616_Serial_Write_Rang(uint8_t address, uint8_t data);
void AD7616_Serial_Set_voltage(uint8_t range);
void AD7616_Seria_Channel(uint8_t channel);
void AD7616_Serial_Output_Format(uint8_t format);
void AD7616_Read_Serial(int16_t *data_A, int16_t *data_B);

#endif