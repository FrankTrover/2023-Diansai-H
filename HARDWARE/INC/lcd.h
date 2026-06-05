#ifndef __LCD_H
#define __LCD_H

#include <stdint.h>

#include "main.h"

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;

#if !defined(LCD_SCL_GPIO_PORT)
#if defined(SCL_GPIO_Port) && defined(SCL_Pin)
#define LCD_SCL_GPIO_PORT SCL_GPIO_Port
#define LCD_SCL_GPIO_PIN SCL_Pin
#elif defined(LCD_SCL_GPIO_Port) && defined(LCD_SCL_Pin)
#define LCD_SCL_GPIO_PORT LCD_SCL_GPIO_Port
#define LCD_SCL_GPIO_PIN LCD_SCL_Pin
#else
#define LCD_SCL_GPIO_PORT GPIOB
#define LCD_SCL_GPIO_PIN GPIO_PIN_13
#endif
#endif

#if !defined(LCD_SDA_GPIO_PORT)
#if defined(SDA_GPIO_Port) && defined(SDA_Pin)
#define LCD_SDA_GPIO_PORT SDA_GPIO_Port
#define LCD_SDA_GPIO_PIN SDA_Pin
#elif defined(LCD_SDA_GPIO_Port) && defined(LCD_SDA_Pin)
#define LCD_SDA_GPIO_PORT LCD_SDA_GPIO_Port
#define LCD_SDA_GPIO_PIN LCD_SDA_Pin
#else
#define LCD_SDA_GPIO_PORT GPIOB
#define LCD_SDA_GPIO_PIN GPIO_PIN_15
#endif
#endif

#if !defined(LCD_RST_GPIO_PORT)
#if defined(RES_GPIO_Port) && defined(RES_Pin)
#define LCD_RST_GPIO_PORT RES_GPIO_Port
#define LCD_RST_GPIO_PIN RES_Pin
#elif defined(LCD_RES_GPIO_Port) && defined(LCD_RES_Pin)
#define LCD_RST_GPIO_PORT LCD_RES_GPIO_Port
#define LCD_RST_GPIO_PIN LCD_RES_Pin
#else
#define LCD_RST_GPIO_PORT GPIOB
#define LCD_RST_GPIO_PIN GPIO_PIN_10
#endif
#endif

#if !defined(LCD_DC_GPIO_PORT)
#if defined(DC_GPIO_Port) && defined(DC_Pin)
#define LCD_DC_GPIO_PORT DC_GPIO_Port
#define LCD_DC_GPIO_PIN DC_Pin
#elif defined(LCD_DC_GPIO_Port) && defined(LCD_DC_Pin)
#define LCD_DC_GPIO_PORT LCD_DC_GPIO_Port
#define LCD_DC_GPIO_PIN LCD_DC_Pin
#else
#define LCD_DC_GPIO_PORT GPIOB
#define LCD_DC_GPIO_PIN GPIO_PIN_14
#endif
#endif

#if !defined(LCD_CS_GPIO_PORT)
#if defined(CS_GPIO_Port) && defined(CS_Pin)
#define LCD_CS_GPIO_PORT CS_GPIO_Port
#define LCD_CS_GPIO_PIN CS_Pin
#elif defined(LCD_CS_GPIO_Port) && defined(LCD_CS_Pin)
#define LCD_CS_GPIO_PORT LCD_CS_GPIO_Port
#define LCD_CS_GPIO_PIN LCD_CS_Pin
#else
#define LCD_CS_GPIO_PORT GPIOB
#define LCD_CS_GPIO_PIN GPIO_PIN_12
#endif
#endif

#if !defined(LCD_BLK_GPIO_PORT)
#if defined(BLK_GPIO_Port) && defined(BLK_Pin)
#define LCD_BLK_GPIO_PORT BLK_GPIO_Port
#define LCD_BLK_GPIO_PIN BLK_Pin
#elif defined(LCD_BLK_GPIO_Port) && defined(LCD_BLK_Pin)
#define LCD_BLK_GPIO_PORT LCD_BLK_GPIO_Port
#define LCD_BLK_GPIO_PIN LCD_BLK_Pin
#else
#define LCD_BLK_GPIO_PORT GPIOB
#define LCD_BLK_GPIO_PIN GPIO_PIN_11
#endif
#endif

#define LCD_SCLK_Clr() HAL_GPIO_WritePin(LCD_SCL_GPIO_PORT, LCD_SCL_GPIO_PIN, GPIO_PIN_RESET)
#define LCD_SCLK_Set() HAL_GPIO_WritePin(LCD_SCL_GPIO_PORT, LCD_SCL_GPIO_PIN, GPIO_PIN_SET)

#define LCD_MOSI_Clr() HAL_GPIO_WritePin(LCD_SDA_GPIO_PORT, LCD_SDA_GPIO_PIN, GPIO_PIN_RESET)
#define LCD_MOSI_Set() HAL_GPIO_WritePin(LCD_SDA_GPIO_PORT, LCD_SDA_GPIO_PIN, GPIO_PIN_SET)

#define LCD_RES_Clr() HAL_GPIO_WritePin(LCD_RST_GPIO_PORT, LCD_RST_GPIO_PIN, GPIO_PIN_RESET)
#define LCD_RES_Set() HAL_GPIO_WritePin(LCD_RST_GPIO_PORT, LCD_RST_GPIO_PIN, GPIO_PIN_SET)

#define LCD_DC_Clr() HAL_GPIO_WritePin(LCD_DC_GPIO_PORT, LCD_DC_GPIO_PIN, GPIO_PIN_RESET)
#define LCD_DC_Set() HAL_GPIO_WritePin(LCD_DC_GPIO_PORT, LCD_DC_GPIO_PIN, GPIO_PIN_SET)

#define LCD_CS_Clr() HAL_GPIO_WritePin(LCD_CS_GPIO_PORT, LCD_CS_GPIO_PIN, GPIO_PIN_RESET)
#define LCD_CS_Set() HAL_GPIO_WritePin(LCD_CS_GPIO_PORT, LCD_CS_GPIO_PIN, GPIO_PIN_SET)

#define LCD_BLK_Clr() HAL_GPIO_WritePin(LCD_BLK_GPIO_PORT, LCD_BLK_GPIO_PIN, GPIO_PIN_RESET)
#define LCD_BLK_Set() HAL_GPIO_WritePin(LCD_BLK_GPIO_PORT, LCD_BLK_GPIO_PIN, GPIO_PIN_SET)

#define LCD_BLK_ACTIVE_HIGH 1

#if LCD_BLK_ACTIVE_HIGH
#define LCD_BLK_On() LCD_BLK_Set()
#define LCD_BLK_Off() LCD_BLK_Clr()
#else
#define LCD_BLK_On() LCD_BLK_Clr()
#define LCD_BLK_Off() LCD_BLK_Set()
#endif

#define USE_HORIZONTAL 1

#if USE_HORIZONTAL == 0 || USE_HORIZONTAL == 1
#define LCD_W 128
#define LCD_H 160
#else
#define LCD_W 160
#define LCD_H 128
#endif

#define WHITE 0xFFFF
#define BLACK 0x0000
#define BLUE 0x001F
#define BRED 0xF81F
#define GRED 0xFFE0
#define GBLUE 0x07FF
#define RED 0xF800
#define MAGENTA 0xF81F
#define GREEN 0x07E0
#define CYAN 0x7FFF
#define YELLOW 0xFFE0
#define BROWN 0xBC40
#define BRRED 0xFC07
#define GRAY 0x8430
#define DARKBLUE 0x01CF
#define LIGHTBLUE 0x7D7C
#define GRAYBLUE 0x5458
#define LIGHTGREEN 0x841F
#define LGRAY 0xC618
#define LGRAYBLUE 0xA651
#define LBBLUE 0x2B12

void LCD_GPIO_Init(void);
void LCD_Writ_Bus(u8 dat);
void LCD_WR_DATA8(u8 dat);
void LCD_WR_DATA(u16 dat);
void LCD_WR_REG(u8 dat);
void LCD_Address_Set(u16 x1, u16 y1, u16 x2, u16 y2);
void LCD_Init(void);
void LCD_Fill(u16 xsta, u16 ysta, u16 xend, u16 yend, u16 color);
void LCD_DrawPoint(u16 x, u16 y, u16 color);
void LCD_DrawLine(u16 x1, u16 y1, u16 x2, u16 y2, u16 color);
void LCD_DrawRectangle(u16 x1, u16 y1, u16 x2, u16 y2, u16 color);
void Draw_Circle(u16 x0, u16 y0, u8 r, u16 color);
void LCD_ShowChinese(u16 x, u16 y, u8 *s, u16 fc, u16 bc, u8 sizey, u8 mode);
void LCD_ShowChinese12x12(u16 x, u16 y, u8 *s, u16 fc, u16 bc, u8 sizey, u8 mode);
void LCD_ShowChinese16x16(u16 x, u16 y, u8 *s, u16 fc, u16 bc, u8 sizey, u8 mode);
void LCD_ShowChinese24x24(u16 x, u16 y, u8 *s, u16 fc, u16 bc, u8 sizey, u8 mode);
void LCD_ShowChinese32x32(u16 x, u16 y, u8 *s, u16 fc, u16 bc, u8 sizey, u8 mode);
void LCD_ShowChar(u16 x, u16 y, u8 num, u16 fc, u16 bc, u8 sizey, u8 mode);
void LCD_ShowString(u16 x, u16 y, const u8 *p, u16 fc, u16 bc, u8 sizey, u8 mode);
u32 mypow(u8 m, u8 n);
void LCD_ShowIntNum(u16 x, u16 y, u16 num, u8 len, u16 fc, u16 bc, u8 sizey);
void LCD_ShowFloatNum1(u16 x, u16 y, float num, u8 len, u16 fc, u16 bc, u8 sizey);
void LCD_ShowPicture(u16 x, u16 y, u16 length, u16 width, const u8 pic[]);

#endif