/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    stm32f4xx_it.c
  * @brief   Interrupt Service Routines.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "stm32f4xx_it.h"
/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <stdbool.h>

#include "dsp_process.h"
#include "lcd.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN TD */

/* USER CODE END TD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

#define TIM2_ADC_BUSY_TIMEOUT_POLLS 20U

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

static void Fault_WriteHexLine(char *line, const char *prefix, uint32_t value)
{
  static const char hex[] = "0123456789ABCDEF";
  uint32_t i;

  line[0] = prefix[0];
  line[1] = prefix[1];
  line[2] = prefix[2];
  line[3] = ':';

  for (i = 0U; i < 8U; ++i)
  {
    line[4U + i] = hex[(value >> (28U - (4U * i))) & 0x0FU];
  }

  line[12] = '\0';
}

/**
  * @brief  All fault handlers funnel here.
  * @param  sp  MSP or PSP at the time of fault.
  * @note   Called from inline assembly in Hard/MemManage/Bus/UsageFault handlers.
  */
__attribute__((used))
static void Fault_CrashHandler(uint32_t sp)
{
  volatile uint32_t lr  = ((volatile uint32_t *)sp)[5];
  volatile uint32_t pc  = ((volatile uint32_t *)sp)[6];

  const char *fault_name;
  uint32_t cfsr = SCB->CFSR;

  /* 根据 CFSR 最高有效标志判断故障类型 */
  if (cfsr & 0x02000000U)      { fault_name = "UsageFault"; }
  else if (cfsr & 0x00800000U) { fault_name = "BusFault";    }
  else if (cfsr & 0x00020000U) { fault_name = "MemManage";   }
  else                         { fault_name = "HardFault";   }

  char line[32];

  LCD_Fill(0, 0, LCD_W, LCD_H, BLACK);
  LCD_ShowString(4U, 4U, (const u8 *)"** CRASH **", RED, BLACK, 16U, 0U);
  LCD_ShowString(4U, 22U, (const u8 *)fault_name, RED, BLACK, 16U, 0U);

  Fault_WriteHexLine(line, "PC ", pc);
  LCD_ShowString(4U, 44U, (const u8 *)line, YELLOW, BLACK, 16U, 0U);

  Fault_WriteHexLine(line, "LR ", lr);
  LCD_ShowString(4U, 62U, (const u8 *)line, YELLOW, BLACK, 16U, 0U);

  Fault_WriteHexLine(line, "CFS", cfsr);
  LCD_ShowString(4U, 80U, (const u8 *)line, YELLOW, BLACK, 16U, 0U);

  /* 红色 LED 持续闪烁表示崩溃 */
  while (1)
  {
    LEC_RED_GPIO_Port->ODR ^= LEC_RED_Pin;
    {
      volatile uint32_t delay;
      for (delay = 0U; delay < 4000000U; ++delay)
      {
        __NOP();
      }
    }
  }
}

/* USER CODE END 0 */

/* External variables --------------------------------------------------------*/
extern TIM_HandleTypeDef htim2;
/* USER CODE BEGIN EV */

extern int16_t g_sample_buffer[DSP_FFT_SIZE];
extern volatile uint32_t g_capture_index;
extern volatile bool g_capture_complete;
extern volatile bool g_capture_error;

/* USER CODE END EV */

/******************************************************************************/
/*           Cortex-M4 Processor Interruption and Exception Handlers          */
/******************************************************************************/
/**
  * @brief This function handles Non maskable interrupt.
  */
void NMI_Handler(void)
{
  /* USER CODE BEGIN NonMaskableInt_IRQn 0 */

  /* USER CODE END NonMaskableInt_IRQn 0 */
  /* USER CODE BEGIN NonMaskableInt_IRQn 1 */
   while (1)
  {
  }
  /* USER CODE END NonMaskableInt_IRQn 1 */
}

/**
  * @brief This function handles Hard fault interrupt.
  */
void HardFault_Handler(void)
{
  /* USER CODE BEGIN HardFault_IRQn 0 */
  __asm volatile(
      "TST LR, #4        \n"
      "ITE EQ            \n"
      "MRSEQ R0, MSP     \n"
      "MRSNE R0, PSP     \n"
      "B Fault_CrashHandler \n"
  );
  /* USER CODE END HardFault_IRQn 0 */
  while (1)
  {
    /* USER CODE BEGIN W1_HardFault_IRQn 0 */
    /* USER CODE END W1_HardFault_IRQn 0 */
  }
}

/**
  * @brief This function handles Memory management fault.
  */
void MemManage_Handler(void)
{
  /* USER CODE BEGIN MemoryManagement_IRQn 0 */
  __asm volatile(
      "TST LR, #4        \n"
      "ITE EQ            \n"
      "MRSEQ R0, MSP     \n"
      "MRSNE R0, PSP     \n"
      "B Fault_CrashHandler \n"
  );
  /* USER CODE END MemoryManagement_IRQn 0 */
  while (1)
  {
    /* USER CODE BEGIN W1_MemoryManagement_IRQn 0 */
    /* USER CODE END W1_MemoryManagement_IRQn 0 */
  }
}

/**
  * @brief This function handles Pre-fetch fault, memory access fault.
  */
void BusFault_Handler(void)
{
  /* USER CODE BEGIN BusFault_IRQn 0 */
  __asm volatile(
      "TST LR, #4        \n"
      "ITE EQ            \n"
      "MRSEQ R0, MSP     \n"
      "MRSNE R0, PSP     \n"
      "B Fault_CrashHandler \n"
  );
  /* USER CODE END BusFault_IRQn 0 */
  while (1)
  {
    /* USER CODE BEGIN W1_BusFault_IRQn 0 */
    /* USER CODE END W1_BusFault_IRQn 0 */
  }
}

/**
  * @brief This function handles Undefined instruction or illegal state.
  */
void UsageFault_Handler(void)
{
  /* USER CODE BEGIN UsageFault_IRQn 0 */
  __asm volatile(
      "TST LR, #4        \n"
      "ITE EQ            \n"
      "MRSEQ R0, MSP     \n"
      "MRSNE R0, PSP     \n"
      "B Fault_CrashHandler \n"
  );
  /* USER CODE END UsageFault_IRQn 0 */
  while (1)
  {
    /* USER CODE BEGIN W1_UsageFault_IRQn 0 */
    /* USER CODE END W1_UsageFault_IRQn 0 */
  }
}

/**
  * @brief This function handles System service call via SWI instruction.
  */
void SVC_Handler(void)
{
  /* USER CODE BEGIN SVCall_IRQn 0 */

  /* USER CODE END SVCall_IRQn 0 */
  /* USER CODE BEGIN SVCall_IRQn 1 */

  /* USER CODE END SVCall_IRQn 1 */
}

/**
  * @brief This function handles Debug monitor.
  */
void DebugMon_Handler(void)
{
  /* USER CODE BEGIN DebugMonitor_IRQn 0 */

  /* USER CODE END DebugMonitor_IRQn 0 */
  /* USER CODE BEGIN DebugMonitor_IRQn 1 */

  /* USER CODE END DebugMonitor_IRQn 1 */
}

/**
  * @brief This function handles Pendable request for system service.
  */
void PendSV_Handler(void)
{
  /* USER CODE BEGIN PendSV_IRQn 0 */

  /* USER CODE END PendSV_IRQn 0 */
  /* USER CODE BEGIN PendSV_IRQn 1 */

  /* USER CODE END PendSV_IRQn 1 */
}

/**
  * @brief This function handles System tick timer.
  */
void SysTick_Handler(void)
{
  /* USER CODE BEGIN SysTick_IRQn 0 */

  /* USER CODE END SysTick_IRQn 0 */
  HAL_IncTick();
  /* USER CODE BEGIN SysTick_IRQn 1 */

  /* USER CODE END SysTick_IRQn 1 */
}

/******************************************************************************/
/* STM32F4xx Peripheral Interrupt Handlers                                    */
/* Add here the Interrupt Handlers for the used peripherals.                  */
/* For the available peripheral interrupt handler names,                      */
/* please refer to the startup file (startup_stm32f4xx.s).                    */
/******************************************************************************/

/**
  * @brief This function handles TIM2 global interrupt.
  */
void TIM2_IRQHandler(void)
{
  /* USER CODE BEGIN TIM2_IRQn 0 */
  if (((TIM2->SR & TIM_SR_UIF) != 0U) && ((TIM2->DIER & TIM_DIER_UIE) != 0U))
  {
    TIM2->SR = (uint32_t)~TIM_SR_UIF;

    if (g_capture_index < DSP_FFT_SIZE)
    {
      uint32_t timeout = TIM2_ADC_BUSY_TIMEOUT_POLLS;

      while (((AD7616_BUSY_GPIO_Port->IDR & AD7616_BUSY_Pin) != 0U) && (--timeout > 0U))
      {
      }

      if (timeout > 0U)
      {
        AD7616_CS_GPIO_Port->BSRR = (uint32_t)AD7616_CS_Pin << 16U;
        AD7616_RD_GPIO_Port->BSRR = (uint32_t)AD7616_RD_Pin << 16U;
        __NOP(); __NOP(); __NOP(); __NOP(); __NOP();
        g_sample_buffer[g_capture_index] = (int16_t)(GPIOD->IDR & 0xFFFFU);
        AD7616_RD_GPIO_Port->BSRR = AD7616_RD_Pin;
        AD7616_CS_GPIO_Port->BSRR = AD7616_CS_Pin;

        g_capture_index++;
        if (g_capture_index >= DSP_FFT_SIZE)
        {
          __DMB();
          g_capture_complete = true;
        }
      }
      else
      {
        g_capture_error = true;
        __DMB();
        g_capture_complete = true;
      }
    }

    return;
  }

  /* USER CODE END TIM2_IRQn 0 */
  HAL_TIM_IRQHandler(&htim2);
  /* USER CODE BEGIN TIM2_IRQn 1 */
  /* USER CODE END TIM2_IRQn 1 */
}

/* USER CODE BEGIN 1 */

/* USER CODE END 1 */
