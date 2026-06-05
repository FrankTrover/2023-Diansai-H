#include "Communication.h"

void DDS_GPIO_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure = {0};

    GPIO_InitStructure.Mode  = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStructure.Pull  = GPIO_NOPULL;
    GPIO_InitStructure.Speed = GPIO_SPEED_FREQ_VERY_HIGH;

    /* AD9833: PA2(FSYNC), PA4(SCLK), PA6(SDATA) */
    __HAL_RCC_GPIOA_CLK_ENABLE();
    GPIO_InitStructure.Pin = GPIO_PIN_2 | GPIO_PIN_4 | GPIO_PIN_6;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStructure);
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_2 | GPIO_PIN_4 | GPIO_PIN_6, GPIO_PIN_SET);

    /* AD9834: PE2(FSY), PE3(FS), PE4(SCK), PE5(PS), PE6(SDA) */
    __HAL_RCC_GPIOE_CLK_ENABLE();
    GPIO_InitStructure.Pin = GPIO_PIN_2 | GPIO_PIN_3 | GPIO_PIN_4 | GPIO_PIN_5 | GPIO_PIN_6;
    HAL_GPIO_Init(GPIOE, &GPIO_InitStructure);
    HAL_GPIO_WritePin(GPIOE, GPIO_PIN_2 | GPIO_PIN_4 | GPIO_PIN_6, GPIO_PIN_SET); /* SPI idle high */
    HAL_GPIO_WritePin(GPIOE, GPIO_PIN_3 | GPIO_PIN_5, GPIO_PIN_RESET);            /* FS/PS default low */

    /* AD9834: PC0(RST) */
    __HAL_RCC_GPIOC_CLK_ENABLE();
    GPIO_InitStructure.Pin = GPIO_PIN_0;
    HAL_GPIO_Init(GPIOC, &GPIO_InitStructure);
    HAL_GPIO_WritePin(GPIOC, GPIO_PIN_0, GPIO_PIN_SET);  /* RST idle high */
}
