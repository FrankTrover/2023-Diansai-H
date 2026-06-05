#ifndef __AD7616_PORT_H__
#define __AD7616_PORT_H__

#include "main.h"

/*
 * Fallback AD7616 pin map used before CubeMX labels are added.
 * Once main.h defines matching AD7616_* macros, those values override these defaults.
 */

#ifndef AD7616_D0_Pin
#define AD7616_D0_Pin GPIO_PIN_0
#define AD7616_D0_GPIO_Port GPIOD
#endif

#ifndef AD7616_D1_Pin
#define AD7616_D1_Pin GPIO_PIN_1
#define AD7616_D1_GPIO_Port GPIOD
#endif

#ifndef AD7616_D2_Pin
#define AD7616_D2_Pin GPIO_PIN_2
#define AD7616_D2_GPIO_Port GPIOD
#endif

#ifndef AD7616_D3_Pin
#define AD7616_D3_Pin GPIO_PIN_3
#define AD7616_D3_GPIO_Port GPIOD
#endif

#ifndef AD7616_D4_Pin
#define AD7616_D4_Pin GPIO_PIN_4
#define AD7616_D4_GPIO_Port GPIOD
#endif

#ifndef AD7616_D5_Pin
#define AD7616_D5_Pin GPIO_PIN_5
#define AD7616_D5_GPIO_Port GPIOD
#endif

#ifndef AD7616_D6_Pin
#define AD7616_D6_Pin GPIO_PIN_6
#define AD7616_D6_GPIO_Port GPIOD
#endif

#ifndef AD7616_D7_Pin
#define AD7616_D7_Pin GPIO_PIN_7
#define AD7616_D7_GPIO_Port GPIOD
#endif

#ifndef AD7616_D8_Pin
#define AD7616_D8_Pin GPIO_PIN_8
#define AD7616_D8_GPIO_Port GPIOD
#endif

#ifndef AD7616_D9_Pin
#define AD7616_D9_Pin GPIO_PIN_9
#define AD7616_D9_GPIO_Port GPIOD
#endif

#ifndef AD7616_D10_Pin
#define AD7616_D10_Pin GPIO_PIN_10
#define AD7616_D10_GPIO_Port GPIOD
#endif

#ifndef AD7616_D11_Pin
#define AD7616_D11_Pin GPIO_PIN_11
#define AD7616_D11_GPIO_Port GPIOD
#endif

#ifndef AD7616_D12_Pin
#define AD7616_D12_Pin GPIO_PIN_12
#define AD7616_D12_GPIO_Port GPIOD
#endif

#ifndef AD7616_D13_Pin
#define AD7616_D13_Pin GPIO_PIN_13
#define AD7616_D13_GPIO_Port GPIOD
#endif

#ifndef AD7616_D14_Pin
#define AD7616_D14_Pin GPIO_PIN_14
#define AD7616_D14_GPIO_Port GPIOD
#endif

#ifndef AD7616_D15_Pin
#define AD7616_D15_Pin GPIO_PIN_15
#define AD7616_D15_GPIO_Port GPIOD
#endif

#ifndef AD7616_CS_Pin
#define AD7616_CS_Pin GPIO_PIN_6
#define AD7616_CS_GPIO_Port GPIOC
#endif

#ifndef AD7616_RD_Pin
#define AD7616_RD_Pin GPIO_PIN_7
#define AD7616_RD_GPIO_Port GPIOC
#endif

#ifndef AD7616_WR_Pin
#define AD7616_WR_Pin GPIO_PIN_8
#define AD7616_WR_GPIO_Port GPIOC
#endif

#ifndef AD7616_CONV_Pin
#define AD7616_CONV_Pin GPIO_PIN_15
#define AD7616_CONV_GPIO_Port GPIOA
#endif

#ifndef AD7616_RST_Pin
#define AD7616_RST_Pin GPIO_PIN_9
#define AD7616_RST_GPIO_Port GPIOC
#endif

#ifndef AD7616_RNG0_Pin
#define AD7616_RNG0_Pin GPIO_PIN_10
#define AD7616_RNG0_GPIO_Port GPIOC
#endif

#ifndef AD7616_RNG1_Pin
#define AD7616_RNG1_Pin GPIO_PIN_11
#define AD7616_RNG1_GPIO_Port GPIOC
#endif

#ifndef AD7616_CHS0_Pin
#define AD7616_CHS0_Pin GPIO_PIN_12
#define AD7616_CHS0_GPIO_Port GPIOC
#endif

#ifndef AD7616_CHS1_Pin
#define AD7616_CHS1_Pin GPIO_PIN_8
#define AD7616_CHS1_GPIO_Port GPIOA
#endif

#ifndef AD7616_CHS2_Pin
#define AD7616_CHS2_Pin GPIO_PIN_9
#define AD7616_CHS2_GPIO_Port GPIOA
#endif

#ifndef AD7616_S_P_Pin
#define AD7616_S_P_Pin GPIO_PIN_10
#define AD7616_S_P_GPIO_Port GPIOA
#endif

#ifndef AD7616_BUSY_Pin
#define AD7616_BUSY_Pin GPIO_PIN_11
#define AD7616_BUSY_GPIO_Port GPIOA
#endif

#endif