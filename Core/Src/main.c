/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
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

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

#include <math.h>
#include <stdbool.h>
#include <string.h>

#include "ad7616.h"
#include "delay.h"
#include "dsp_process.h"
#include "lcd_ui.h"
#include "signal_output.h"

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

typedef enum
{
  APP_STATE_IDLE = 0,
  APP_STATE_ACQUIRE,
  APP_STATE_ANALYZE,
  APP_STATE_OUTPUT,
  APP_STATE_ERROR
} AppState_t;

typedef enum
{
  APP_SCENE_UNKNOWN = 0,
  APP_SCENE_BASIC_SINE_50K_100K,
  APP_SCENE_ADVANCED_SINE_10K_GRID,
  APP_SCENE_ADVANCED_MIXED_5K_GRID,
  APP_SCENE_GENERIC
} AppScene_t;

typedef struct
{
  bool valid;
  uint32_t freq_hz;
  DSP_WaveformType_t wave_type;
  float vpp;
} AppSeparatedSignal_t;

typedef struct
{
  bool valid;
  bool locked;
  bool dual_output_required;
  bool dual_output_supported;
  bool degraded_output;
  uint32_t lock_tick;
  AppScene_t scene;
  AppSeparatedSignal_t signal_a;
  AppSeparatedSignal_t signal_b;
} AppSplitPlan_t;

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/*
 * 实际采样率 = TIM2_CLK / (Period + 1) = 84MHz / 168 = 500kHz
 * 注意：APB1=42MHz，定时器时钟 ×2 = 84MHz。Period=167 不是 1MHz！
 */
#define APP_SAMPLE_RATE_HZ             500000.0f
#define APP_RESTART_DELAY_MS           3000U
#define APP_MAX_SEPARATION_TIME_MS     20000U
#define APP_LOCK_VALID_MS              5000U
#define APP_FREQ_MATCH_TOLERANCE_HZ    800.0f
#define APP_GRID_10K_HZ                10000.0f
#define APP_GRID_5K_HZ                 5000.0f
#define APP_LED_ON                     GPIO_PIN_RESET
#define APP_LED_OFF                    GPIO_PIN_SET

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
TIM_HandleTypeDef htim2;

/* USER CODE BEGIN PV */

static AppState_t g_app_state = APP_STATE_IDLE;
int16_t g_sample_buffer[DSP_FFT_SIZE] __attribute__((aligned(4)));
volatile uint32_t g_capture_index = 0U;
volatile bool g_capture_complete = false;
volatile bool g_capture_error = false;
static AppSplitPlan_t g_split_plan;
static float g_requested_vpp = SIGNAL_OUTPUT_FULL_SCALE_VPP;
static uint32_t g_next_capture_tick = 0U;
static uint32_t g_task_start_tick = 0U;
static const char *g_last_error_message = "SEARCHING";

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_TIM2_Init(void);
/* USER CODE BEGIN PFP */

static void App_Init(void);
static void App_SetStatusLeds(bool busy, bool error);
static void App_StartCaptureTimer(void);
static void App_StopCaptureTimer(void);
static HAL_StatusTypeDef App_CaptureFrame(void);
static HAL_StatusTypeDef App_ProcessFrame(void);
static bool App_IsNearFrequency(float value_hz, float target_hz);
static bool App_IsGridMultiple(float value_hz, float step_hz);
static const SignalComponent_t *App_GetLowerFrequencyComponent(const SignalAnalysisResult_t *analysis);
static const SignalComponent_t *App_GetHigherFrequencyComponent(const SignalAnalysisResult_t *analysis);
static DSP_WaveformType_t App_NormalizeOutputWaveform(DSP_WaveformType_t wave_type);
static AppScene_t App_ClassifyScene(const SignalAnalysisResult_t *analysis);
static HAL_StatusTypeDef App_UpdateSplitPlan(void);
static HAL_StatusTypeDef App_ApplyOutputPlan(void);
static bool App_IsTaskTimeout(void);
static bool App_IsSplitPlanFresh(void);

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
  /*
   * TIM2 runs at 500 kHz for AD7616 sampling and is handled directly in
   * TIM2_IRQHandler() to avoid HAL callback overhead. Keep this callback empty
   * so a future HAL path cannot duplicate samples.
   */
  (void)htim;
}

static bool App_IsNearFrequency(float value_hz, float target_hz)
{
  return fabsf(value_hz - target_hz) <= APP_FREQ_MATCH_TOLERANCE_HZ;
}

static bool App_IsGridMultiple(float value_hz, float step_hz)
{
  float remainder;
  float distance;

  if (step_hz <= 0.0f)
  {
    return false;
  }

  remainder = fmodf(value_hz, step_hz);
  if (remainder < 0.0f)
  {
    remainder = -remainder;
  }

  distance = remainder;
  if ((step_hz - remainder) < distance)
  {
    distance = step_hz - remainder;
  }

  return distance <= APP_FREQ_MATCH_TOLERANCE_HZ;
}

static const SignalComponent_t *App_GetLowerFrequencyComponent(const SignalAnalysisResult_t *analysis)
{
  if ((analysis == NULL) || !analysis->primary.valid || !analysis->secondary.valid)
  {
    return NULL;
  }

  if (analysis->primary.frequency_hz <= analysis->secondary.frequency_hz)
  {
    return &analysis->primary;
  }

  return &analysis->secondary;
}

static const SignalComponent_t *App_GetHigherFrequencyComponent(const SignalAnalysisResult_t *analysis)
{
  if ((analysis == NULL) || !analysis->primary.valid || !analysis->secondary.valid)
  {
    return NULL;
  }

  if (analysis->primary.frequency_hz > analysis->secondary.frequency_hz)
  {
    return &analysis->primary;
  }

  return &analysis->secondary;
}

static DSP_WaveformType_t App_NormalizeOutputWaveform(DSP_WaveformType_t wave_type)
{
  switch (wave_type)
  {
  case DSP_WAVE_SINE:
  case DSP_WAVE_TRIANGLE:
  case DSP_WAVE_SQUARE:
    return wave_type;
  case DSP_WAVE_UNKNOWN:
  default:
    return DSP_WAVE_SINE;
  }
}

static AppScene_t App_ClassifyScene(const SignalAnalysisResult_t *analysis)
{
  const SignalComponent_t *signal_a = App_GetLowerFrequencyComponent(analysis);
  const SignalComponent_t *signal_b = App_GetHigherFrequencyComponent(analysis);

  if ((signal_a == NULL) || (signal_b == NULL))
  {
    return APP_SCENE_UNKNOWN;
  }

  if ((signal_a->frequency_hz < DSP_MIN_ANALYSIS_FREQ_HZ) ||
      (signal_b->frequency_hz > DSP_MAX_ANALYSIS_FREQ_HZ))
  {
    return APP_SCENE_UNKNOWN;
  }

  if (App_IsNearFrequency(signal_a->frequency_hz, 50000.0f) &&
      App_IsNearFrequency(signal_b->frequency_hz, 100000.0f))
  {
    return APP_SCENE_BASIC_SINE_50K_100K;
  }

  if ((signal_a->wave_type == DSP_WAVE_SINE) &&
      (signal_b->wave_type == DSP_WAVE_SINE) &&
      App_IsGridMultiple(signal_a->frequency_hz, APP_GRID_10K_HZ) &&
      App_IsGridMultiple(signal_b->frequency_hz, APP_GRID_10K_HZ))
  {
    return APP_SCENE_ADVANCED_SINE_10K_GRID;
  }

  if (((signal_a->wave_type == DSP_WAVE_SINE) || (signal_a->wave_type == DSP_WAVE_TRIANGLE)) &&
      ((signal_b->wave_type == DSP_WAVE_SINE) || (signal_b->wave_type == DSP_WAVE_TRIANGLE)) &&
      App_IsGridMultiple(signal_a->frequency_hz, APP_GRID_5K_HZ) &&
      App_IsGridMultiple(signal_b->frequency_hz, APP_GRID_5K_HZ))
  {
    return APP_SCENE_ADVANCED_MIXED_5K_GRID;
  }

  return APP_SCENE_GENERIC;
}

static bool App_IsTaskTimeout(void)
{
  return (!g_split_plan.locked) && ((HAL_GetTick() - g_task_start_tick) > APP_MAX_SEPARATION_TIME_MS);
}

static bool App_IsSplitPlanFresh(void)
{
  return g_split_plan.locked && ((HAL_GetTick() - g_split_plan.lock_tick) <= APP_LOCK_VALID_MS);
}

/**
 * 频率栅格吸附：将 FFT 估计值四舍五入到最近的 grid_hz 整数倍。
 *
 * FFT 在 500kSPS/2048 点下 bin 间隔 ≈ 244Hz，即使抛物线插值仍有
 * 几十到几百 Hz 误差。题目频率是 10kHz 或 5kHz 整数倍，吸附后
 * 输出与原信号精确同频，示波器上不漂移。
 *
 * 吸附容差：APP_FREQ_MATCH_TOLERANCE_HZ (800Hz)。
 * 超出容差则原样返回（不吸附）。
 */
static uint32_t App_SnapToGrid(float freq_hz, float grid_hz)
{
  float rounded;

  if (grid_hz <= 0.0f)
  {
    return (uint32_t)(freq_hz + 0.5f);
  }

  rounded = roundf(freq_hz / grid_hz) * grid_hz;

  if (fabsf(rounded - freq_hz) <= APP_FREQ_MATCH_TOLERANCE_HZ)
  {
    return (uint32_t)(rounded + 0.5f);
  }

  return (uint32_t)(freq_hz + 0.5f);
}

static void App_Init(void)
{
  memset(&g_split_plan, 0, sizeof(g_split_plan));
  delay_init(HAL_RCC_GetSysClockFreq());
  AD7616_Init(HARDWARE_MODE);
  AD7616_Reset();
  AD7616_Parallel_Set_voltage(Range_5_V);
  AD7616_Parallel_Channel_Select(0U);

  Signal_Output_Init();
  DSP_Process_Init();
  LCD_UI_Init();
  App_SetStatusLeds(false, false);
  g_task_start_tick = HAL_GetTick();
  g_next_capture_tick = g_task_start_tick;
  g_last_error_message = "SEARCHING";
}

static void App_SetStatusLeds(bool busy, bool error)
{
  HAL_GPIO_WritePin(LED_BULL_GPIO_Port, LED_BULL_Pin, busy ? APP_LED_ON : APP_LED_OFF);
  HAL_GPIO_WritePin(LEC_RED_GPIO_Port, LEC_RED_Pin, error ? APP_LED_ON : APP_LED_OFF);
}

static void App_StartCaptureTimer(void)
{
  TIM2->DIER &= (uint32_t)~TIM_DIER_UIE;
  TIM2->CR1 &= (uint32_t)~TIM_CR1_CEN;
  TIM2->CCER &= (uint32_t)~TIM_CCER_CC1E;
  TIM2->CNT = 0U;
  TIM2->EGR = TIM_EGR_UG;
  TIM2->SR = 0U;
  TIM2->CCER |= TIM_CCER_CC1E;
  TIM2->DIER |= TIM_DIER_UIE;
  TIM2->CR1 |= TIM_CR1_CEN;
}

static void App_StopCaptureTimer(void)
{
  TIM2->DIER &= (uint32_t)~TIM_DIER_UIE;
  TIM2->CR1 &= (uint32_t)~TIM_CR1_CEN;
  TIM2->CCER &= (uint32_t)~TIM_CCER_CC1E;
  TIM2->SR = 0U;
}

static HAL_StatusTypeDef App_CaptureFrame(void)
{
  /*
   * 信号采集模块入口：TIM2 中断驱动的 500kSPS 采集。
   *
   * TIM2 PWM 自动在 PA15 产生 500kHz CONV 脉冲，无需软件控制。
   * TIM2 Update 中断在每次 CONV↓ ~1μs 后触发（转换已完成）：
   *   1. CS↓ + RD↓ 拉低控制线
   *   2. 读 GPIOD->IDR 获取 16 位数据
   *   3. RD↑ + CS↑ 恢复总线
   * 当采集满 2048 点后，g_capture_complete = true，停止 TIM2。
   */
  /* Keep the LCD on the single result page; do not show an acquiring page. */
  App_SetStatusLeds(true, false);

  g_capture_index = 0U;
  g_capture_complete = false;
  g_capture_error = false;
  __DMB();

  /* 启动 TIM2 PWM → PA15 自动产生 CONV 脉冲 */
  App_StartCaptureTimer();

  /* 等待采集完成（TIM2 中断填满 buffer 后置位标志） */
  {
    uint32_t wait_start = HAL_GetTick();
    while (true)
    {
      __DMB();
      if (g_capture_complete)
      {
        break;
      }

      if ((HAL_GetTick() - wait_start) > 100U) /* 超时 100ms（2048 点约 4.1ms） */
      {
        App_StopCaptureTimer();
        g_last_error_message = "CAPTURE TIMEOUT";
        return HAL_TIMEOUT;
      }
    }
  }

  /* 停止 TIM2 */
  App_StopCaptureTimer();

  __DMB();
  if (g_capture_error)
  {
    g_last_error_message = "ADC BUSY TIMEOUT";
    return HAL_ERROR;
  }

  return HAL_OK;
}

static HAL_StatusTypeDef App_ProcessFrame(void)
{
  /* 算法处理模块入口：在此完成 FFT、谱峰搜索、场景识别和分离决策。 */
  return DSP_ProcessSamples(g_sample_buffer, DSP_FFT_SIZE, APP_SAMPLE_RATE_HZ);
}

static HAL_StatusTypeDef App_UpdateSplitPlan(void)
{
  AppSplitPlan_t next_plan;
  const SignalAnalysisResult_t *analysis = DSP_GetLastAnalysis();
  const SignalComponent_t *signal_a = App_GetLowerFrequencyComponent(analysis);
  const SignalComponent_t *signal_b = App_GetHigherFrequencyComponent(analysis);

  memset(&next_plan, 0, sizeof(next_plan));

  if ((analysis == NULL) || !analysis->valid || (signal_a == NULL) || (signal_b == NULL))
  {
    g_last_error_message = "WAITING PEAKS";
    return HAL_BUSY;
  }

  next_plan.scene = App_ClassifyScene(analysis);
  if (next_plan.scene == APP_SCENE_UNKNOWN)
  {
    g_last_error_message = "SCENE UNSUPPORTED";
    return HAL_BUSY;
  }

  next_plan.valid = true;
  next_plan.locked = true;
  next_plan.dual_output_required = true;
  next_plan.dual_output_supported = true;
  next_plan.degraded_output = false;
  next_plan.lock_tick = HAL_GetTick();

  next_plan.signal_a.valid = true;
  next_plan.signal_a.wave_type = App_NormalizeOutputWaveform(signal_a->wave_type);
  next_plan.signal_a.vpp = g_requested_vpp;

  next_plan.signal_b.valid = true;
  next_plan.signal_b.wave_type = App_NormalizeOutputWaveform(signal_b->wave_type);
  next_plan.signal_b.vpp = g_requested_vpp;

  /*
   * 频率栅格吸附：根据识别的场景将 FFT 估计值锁定到精确栅格。
   * - 基本项(50k+100k)：吸附到 50kHz 和 100kHz
   * - 10kHz 栅格：吸附到 10kHz 整数倍
   * - 5kHz 栅格：吸附到 5kHz 整数倍
   * 消除 FFT 频率估计误差导致的示波器漂移。
   */
  switch (next_plan.scene)
  {
  case APP_SCENE_BASIC_SINE_50K_100K:
    next_plan.signal_a.freq_hz = 50000U;
    next_plan.signal_b.freq_hz = 100000U;
    break;
  case APP_SCENE_ADVANCED_SINE_10K_GRID:
    next_plan.signal_a.freq_hz = App_SnapToGrid(signal_a->frequency_hz, APP_GRID_10K_HZ);
    next_plan.signal_b.freq_hz = App_SnapToGrid(signal_b->frequency_hz, APP_GRID_10K_HZ);
    break;
  case APP_SCENE_ADVANCED_MIXED_5K_GRID:
    next_plan.signal_a.freq_hz = App_SnapToGrid(signal_a->frequency_hz, APP_GRID_5K_HZ);
    next_plan.signal_b.freq_hz = App_SnapToGrid(signal_b->frequency_hz, APP_GRID_5K_HZ);
    break;
  case APP_SCENE_GENERIC:
    next_plan.signal_a.freq_hz = (uint32_t)(signal_a->frequency_hz + 0.5f);
    next_plan.signal_b.freq_hz = (uint32_t)(signal_b->frequency_hz + 0.5f);
    break;
  default:
    next_plan.signal_a.freq_hz = (uint32_t)(signal_a->frequency_hz + 0.5f);
    next_plan.signal_b.freq_hz = (uint32_t)(signal_b->frequency_hz + 0.5f);
    break;
  }

  g_split_plan = next_plan;
  g_last_error_message = "DUAL OUTPUT OK";
  return HAL_OK;
}

static HAL_StatusTypeDef App_ApplyOutputPlan(void)
{
  /* 信号输出模块入口：在此根据分离结果驱动两片 DDS 同时输出 A' 和 B'。 */
  if (!g_split_plan.valid || !g_split_plan.signal_a.valid || !g_split_plan.signal_b.valid)
  {
    return HAL_ERROR;
  }

  /*
   * 双路 DDS 同时输出：
   *   AD9833 输出信号 A'（低频分量）
   *   AD9834 输出信号 B'（高频分量）
   * Set_Waveform_Output 内部根据频率自动选择合适的 DDS 芯片。
   * 这里强制指定：AD9833 走低频，AD9834 走高频。
   */
  if (!Set_Waveform_Output(g_split_plan.signal_a.freq_hz,
                           (uint8_t)g_split_plan.signal_a.wave_type,
                           g_split_plan.signal_a.vpp))
  {
    g_last_error_message = "DDS-A FAIL";
    return HAL_ERROR;
  }

  if (!Set_Waveform_Output_Dual(g_split_plan.signal_b.freq_hz,
                                (uint8_t)g_split_plan.signal_b.wave_type,
                                g_split_plan.signal_b.vpp))
  {
    g_last_error_message = "DDS-B FAIL";
    return HAL_ERROR;
  }

  return HAL_OK;
}

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_TIM2_Init();
  /* USER CODE BEGIN 2 */

  App_Init();

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
    switch (g_app_state)
    {
    case APP_STATE_IDLE:
      if ((int32_t)(HAL_GetTick() - g_next_capture_tick) >= 0)
      {
        g_app_state = APP_STATE_ACQUIRE;
      }
      break;

    case APP_STATE_ACQUIRE:
      if (App_CaptureFrame() == HAL_OK)
      {
        g_app_state = APP_STATE_ANALYZE;
      }
      else
      {
        g_app_state = APP_STATE_ERROR;
      }
      break;

    case APP_STATE_ANALYZE:
      if (App_ProcessFrame() == HAL_OK)
      {
        HAL_StatusTypeDef plan_status = App_UpdateSplitPlan();

        if ((plan_status == HAL_OK) || App_IsSplitPlanFresh())
        {
          g_app_state = APP_STATE_OUTPUT;
        }
        else if (App_IsTaskTimeout())
        {
          g_last_error_message = "TIMEOUT >20S";
          g_app_state = APP_STATE_ERROR;
        }
        else
        {
          g_next_capture_tick = HAL_GetTick() + APP_RESTART_DELAY_MS;
          g_app_state = APP_STATE_IDLE;
        }
      }
      else
      {
        g_app_state = APP_STATE_ERROR;
      }
      break;

    case APP_STATE_OUTPUT:
    {
      LCD_SplitResult_t lcd_result;

      if (App_ApplyOutputPlan() != HAL_OK)
      {
        g_last_error_message = "DDS OUTPUT FAIL";
        g_app_state = APP_STATE_ERROR;
        break;
      }

      /* 填充 LCD 显示数据：输入端检测结果 + DDS 输出端实际值 */
      lcd_result.valid = true;
      lcd_result.input_a.valid = g_split_plan.signal_a.valid;
      lcd_result.input_a.freq_hz = g_split_plan.signal_a.freq_hz;
      lcd_result.input_a.wave_type = g_split_plan.signal_a.wave_type;
      lcd_result.input_b.valid = g_split_plan.signal_b.valid;
      lcd_result.input_b.freq_hz = g_split_plan.signal_b.freq_hz;
      lcd_result.input_b.wave_type = g_split_plan.signal_b.wave_type;
      lcd_result.dds_a.valid = true;
      lcd_result.dds_a.freq_hz = g_split_plan.signal_a.freq_hz;
      lcd_result.dds_a.wave_type = g_split_plan.signal_a.wave_type;
      lcd_result.dds_b.valid = true;
      lcd_result.dds_b.freq_hz = g_split_plan.signal_b.freq_hz;
      lcd_result.dds_b.wave_type = g_split_plan.signal_b.wave_type;

      LCD_UI_ShowSplitResult(&lcd_result);
      App_SetStatusLeds(false, false);
      g_next_capture_tick = HAL_GetTick() + APP_RESTART_DELAY_MS;
      g_app_state = APP_STATE_IDLE;
      break;
    }

    case APP_STATE_ERROR:
      LCD_UI_ShowError(g_last_error_message);
      App_SetStatusLeds(false, true);
      g_next_capture_tick = HAL_GetTick() + APP_RESTART_DELAY_MS;
      delay_ms(200U);
      g_app_state = APP_STATE_IDLE;
      break;

    default:
      g_app_state = APP_STATE_IDLE;
      break;
    }
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage
  */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 25;
  RCC_OscInitStruct.PLL.PLLN = 336;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 4;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_5) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief TIM2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM2_Init(void)
{

  /* USER CODE BEGIN TIM2_Init 0 */

  /* USER CODE END TIM2_Init 0 */

  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};
  TIM_OC_InitTypeDef sConfigOC = {0};

  /* USER CODE BEGIN TIM2_Init 1 */

  /* USER CODE END TIM2_Init 1 */
  htim2.Instance = TIM2;
  htim2.Init.Prescaler = 0;
  htim2.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim2.Init.Period = 167;
  htim2.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim2.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim2) != HAL_OK)
  {
    Error_Handler();
  }
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim2, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_TIM_PWM_Init(&htim2) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim2, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sConfigOC.OCMode = TIM_OCMODE_PWM1;
  sConfigOC.Pulse = 83;
  sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
  sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
  if (HAL_TIM_PWM_ConfigChannel(&htim2, &sConfigOC, TIM_CHANNEL_1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM2_Init 2 */

  /* USER CODE END TIM2_Init 2 */
  HAL_TIM_MspPostInit(&htim2);

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  /* USER CODE BEGIN MX_GPIO_Init_1 */

  /* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOE_CLK_ENABLE();
  __HAL_RCC_GPIOH_CLK_ENABLE();
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();
  __HAL_RCC_GPIOD_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOE, AD9834_FSY_Pin|AD9834_FS_Pin|AD9834_SCK_Pin|AD9834_PS_Pin
                          |AD9834_SDA_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOC, AD9834_RST_Pin|LEC_RED_Pin|AD7616_CS_Pin|AD7616_RD_Pin
                          |AD7616_WR_Pin|AD7616_RST_Pin|AD7616_RNG0_Pin|AD7616_RNG1_Pin
                          |AD7616_CHS0_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOA, AD9833_FSYNC_Pin|AD9833_SCLK_Pin|AD9833_SDATA_Pin|AD7616_CHS1_Pin
                          |AD7616_CHS2_Pin|AD7616_S_P_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOB, LED_BULL_Pin|LCD_RES_Pin|LCD_BLK_Pin|LCD_CS_Pin
                          |LCD_SCL_Pin|LCD_DC_Pin|LCD_SDA_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pins : AD9834_FSY_Pin AD9834_FS_Pin AD9834_SCK_Pin AD9834_PS_Pin
                           AD9834_SDA_Pin */
  GPIO_InitStruct.Pin = AD9834_FSY_Pin|AD9834_FS_Pin|AD9834_SCK_Pin|AD9834_PS_Pin
                          |AD9834_SDA_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
  HAL_GPIO_Init(GPIOE, &GPIO_InitStruct);

  /*Configure GPIO pins : AD9834_RST_Pin LEC_RED_Pin AD7616_CS_Pin AD7616_RD_Pin
                           AD7616_WR_Pin AD7616_RST_Pin AD7616_RNG0_Pin AD7616_RNG1_Pin
                           AD7616_CHS0_Pin */
  GPIO_InitStruct.Pin = AD9834_RST_Pin|LEC_RED_Pin|AD7616_CS_Pin|AD7616_RD_Pin
                          |AD7616_WR_Pin|AD7616_RST_Pin|AD7616_RNG0_Pin|AD7616_RNG1_Pin
                          |AD7616_CHS0_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  /*Configure GPIO pins : KEY_Pin AD7616_BUSY_Pin */
  GPIO_InitStruct.Pin = KEY_Pin|AD7616_BUSY_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pins : AD9833_FSYNC_Pin AD9833_SCLK_Pin AD9833_SDATA_Pin AD7616_CHS1_Pin
                           AD7616_CHS2_Pin AD7616_S_P_Pin */
  GPIO_InitStruct.Pin = AD9833_FSYNC_Pin|AD9833_SCLK_Pin|AD9833_SDATA_Pin|AD7616_CHS1_Pin
                          |AD7616_CHS2_Pin|AD7616_S_P_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pins : LED_BULL_Pin LCD_RES_Pin LCD_BLK_Pin LCD_CS_Pin
                           LCD_SCL_Pin LCD_DC_Pin LCD_SDA_Pin */
  GPIO_InitStruct.Pin = LED_BULL_Pin|LCD_RES_Pin|LCD_BLK_Pin|LCD_CS_Pin
                          |LCD_SCL_Pin|LCD_DC_Pin|LCD_SDA_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /*Configure GPIO pins : AD7616_D8_Pin AD7616_D9_Pin AD7616_D10_Pin AD7616_D11_Pin
                           AD7616_D12_Pin AD7616_D13_Pin AD7616_D14_Pin AD7616_D15_Pin
                           AD7616_D0_Pin AD7616_D1_Pin AD7616_D2_Pin AD7616_D3_Pin
                           AD7616_D4_Pin AD7616_D5_Pin AD7616_D6_Pin AD7616_D7_Pin */
  GPIO_InitStruct.Pin = AD7616_D8_Pin|AD7616_D9_Pin|AD7616_D10_Pin|AD7616_D11_Pin
                          |AD7616_D12_Pin|AD7616_D13_Pin|AD7616_D14_Pin|AD7616_D15_Pin
                          |AD7616_D0_Pin|AD7616_D1_Pin|AD7616_D2_Pin|AD7616_D3_Pin
                          |AD7616_D4_Pin|AD7616_D5_Pin|AD7616_D6_Pin|AD7616_D7_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);

  /* USER CODE BEGIN MX_GPIO_Init_2 */

  /* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}
#ifdef USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
