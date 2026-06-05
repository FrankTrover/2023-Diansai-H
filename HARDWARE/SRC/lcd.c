#include "lcd.h"

#include "delay.h"
#include "lcdfont.h"

static void LCD_Enable_GPIO_Clock(GPIO_TypeDef *port)
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

void LCD_GPIO_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure = {0};

    LCD_Enable_GPIO_Clock(LCD_SCL_GPIO_PORT);
    LCD_Enable_GPIO_Clock(LCD_SDA_GPIO_PORT);
    LCD_Enable_GPIO_Clock(LCD_RST_GPIO_PORT);
    LCD_Enable_GPIO_Clock(LCD_DC_GPIO_PORT);
    LCD_Enable_GPIO_Clock(LCD_CS_GPIO_PORT);
    LCD_Enable_GPIO_Clock(LCD_BLK_GPIO_PORT);

    GPIO_InitStructure.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStructure.Pull = GPIO_NOPULL;
    GPIO_InitStructure.Speed = GPIO_SPEED_FREQ_VERY_HIGH;

    GPIO_InitStructure.Pin = LCD_SCL_GPIO_PIN;
    HAL_GPIO_Init(LCD_SCL_GPIO_PORT, &GPIO_InitStructure);
    GPIO_InitStructure.Pin = LCD_SDA_GPIO_PIN;
    HAL_GPIO_Init(LCD_SDA_GPIO_PORT, &GPIO_InitStructure);
    GPIO_InitStructure.Pin = LCD_RST_GPIO_PIN;
    HAL_GPIO_Init(LCD_RST_GPIO_PORT, &GPIO_InitStructure);
    GPIO_InitStructure.Pin = LCD_DC_GPIO_PIN;
    HAL_GPIO_Init(LCD_DC_GPIO_PORT, &GPIO_InitStructure);
    GPIO_InitStructure.Pin = LCD_CS_GPIO_PIN;
    HAL_GPIO_Init(LCD_CS_GPIO_PORT, &GPIO_InitStructure);
    GPIO_InitStructure.Pin = LCD_BLK_GPIO_PIN;
    HAL_GPIO_Init(LCD_BLK_GPIO_PORT, &GPIO_InitStructure);

    LCD_SCLK_Set();
    LCD_MOSI_Set();
    LCD_RES_Set();
    LCD_DC_Set();
    LCD_CS_Set();
    LCD_BLK_Off();
}

void LCD_Writ_Bus(u8 dat)
{
    u8 i;

    LCD_CS_Clr();
    for (i = 0; i < 8; i++)
    {
        LCD_SCLK_Clr();
        if (dat & 0x80)
        {
            LCD_MOSI_Set();
        }
        else
        {
            LCD_MOSI_Clr();
        }
        LCD_SCLK_Set();
        dat <<= 1;
    }
    LCD_CS_Set();
}

void LCD_WR_DATA8(u8 dat)
{
    LCD_Writ_Bus(dat);
}

void LCD_WR_DATA(u16 dat)
{
    LCD_Writ_Bus((u8)(dat >> 8));
    LCD_Writ_Bus((u8)dat);
}

void LCD_WR_REG(u8 dat)
{
    LCD_DC_Clr();
    LCD_Writ_Bus(dat);
    LCD_DC_Set();
}

void LCD_Address_Set(u16 x1, u16 y1, u16 x2, u16 y2)
{
    if (USE_HORIZONTAL == 0)
    {
        LCD_WR_REG(0x2a);
        LCD_WR_DATA(x1 + 2);
        LCD_WR_DATA(x2 + 2);
        LCD_WR_REG(0x2b);
        LCD_WR_DATA(y1 + 1);
        LCD_WR_DATA(y2 + 1);
        LCD_WR_REG(0x2c);
    }
    else if (USE_HORIZONTAL == 1)
    {
        LCD_WR_REG(0x2a);
        LCD_WR_DATA(x1 + 2);
        LCD_WR_DATA(x2 + 2);
        LCD_WR_REG(0x2b);
        LCD_WR_DATA(y1 + 1);
        LCD_WR_DATA(y2 + 1);
        LCD_WR_REG(0x2c);
    }
    else if (USE_HORIZONTAL == 2)
    {
        LCD_WR_REG(0x2a);
        LCD_WR_DATA(x1 + 1);
        LCD_WR_DATA(x2 + 1);
        LCD_WR_REG(0x2b);
        LCD_WR_DATA(y1 + 2);
        LCD_WR_DATA(y2 + 2);
        LCD_WR_REG(0x2c);
    }
    else
    {
        LCD_WR_REG(0x2a);
        LCD_WR_DATA(x1 + 1);
        LCD_WR_DATA(x2 + 1);
        LCD_WR_REG(0x2b);
        LCD_WR_DATA(y1 + 2);
        LCD_WR_DATA(y2 + 2);
        LCD_WR_REG(0x2c);
    }
}

void LCD_Init(void)
{
    LCD_GPIO_Init();

    LCD_RES_Clr();
    delay_ms(100);
    LCD_RES_Set();
    delay_ms(100);

    LCD_BLK_On();
    delay_ms(100);

    LCD_WR_REG(0x11);
    delay_ms(120);
    LCD_WR_REG(0xB1);
    LCD_WR_DATA8(0x05);
    LCD_WR_DATA8(0x3C);
    LCD_WR_DATA8(0x3C);
    LCD_WR_REG(0xB2);
    LCD_WR_DATA8(0x05);
    LCD_WR_DATA8(0x3C);
    LCD_WR_DATA8(0x3C);
    LCD_WR_REG(0xB3);
    LCD_WR_DATA8(0x05);
    LCD_WR_DATA8(0x3C);
    LCD_WR_DATA8(0x3C);
    LCD_WR_DATA8(0x05);
    LCD_WR_DATA8(0x3C);
    LCD_WR_DATA8(0x3C);
    LCD_WR_REG(0xB4);
    LCD_WR_DATA8(0x03);
    LCD_WR_REG(0xC0);
    LCD_WR_DATA8(0x28);
    LCD_WR_DATA8(0x08);
    LCD_WR_DATA8(0x04);
    LCD_WR_REG(0xC1);
    LCD_WR_DATA8(0xC0);
    LCD_WR_REG(0xC2);
    LCD_WR_DATA8(0x0D);
    LCD_WR_DATA8(0x00);
    LCD_WR_REG(0xC3);
    LCD_WR_DATA8(0x8D);
    LCD_WR_DATA8(0x2A);
    LCD_WR_REG(0xC4);
    LCD_WR_DATA8(0x8D);
    LCD_WR_DATA8(0xEE);
    LCD_WR_REG(0xC5);
    LCD_WR_DATA8(0x1A);
    LCD_WR_REG(0x36);
    if (USE_HORIZONTAL == 0)
    {
        LCD_WR_DATA8(0x00);
    }
    else if (USE_HORIZONTAL == 1)
    {
        LCD_WR_DATA8(0xC0);
    }
    else if (USE_HORIZONTAL == 2)
    {
        LCD_WR_DATA8(0x70);
    }
    else
    {
        LCD_WR_DATA8(0xA0);
    }
    LCD_WR_REG(0xE0);
    LCD_WR_DATA8(0x04);
    LCD_WR_DATA8(0x22);
    LCD_WR_DATA8(0x07);
    LCD_WR_DATA8(0x0A);
    LCD_WR_DATA8(0x2E);
    LCD_WR_DATA8(0x30);
    LCD_WR_DATA8(0x25);
    LCD_WR_DATA8(0x2A);
    LCD_WR_DATA8(0x28);
    LCD_WR_DATA8(0x26);
    LCD_WR_DATA8(0x2E);
    LCD_WR_DATA8(0x3A);
    LCD_WR_DATA8(0x00);
    LCD_WR_DATA8(0x01);
    LCD_WR_DATA8(0x03);
    LCD_WR_DATA8(0x13);
    LCD_WR_REG(0xE1);
    LCD_WR_DATA8(0x04);
    LCD_WR_DATA8(0x16);
    LCD_WR_DATA8(0x06);
    LCD_WR_DATA8(0x0D);
    LCD_WR_DATA8(0x2D);
    LCD_WR_DATA8(0x26);
    LCD_WR_DATA8(0x23);
    LCD_WR_DATA8(0x27);
    LCD_WR_DATA8(0x27);
    LCD_WR_DATA8(0x25);
    LCD_WR_DATA8(0x2D);
    LCD_WR_DATA8(0x3B);
    LCD_WR_DATA8(0x00);
    LCD_WR_DATA8(0x01);
    LCD_WR_DATA8(0x04);
    LCD_WR_DATA8(0x13);
    LCD_WR_REG(0x3A);
    LCD_WR_DATA8(0x05);
    LCD_WR_REG(0x29);

    LCD_Fill(0, 0, LCD_W, LCD_H, BLACK);
}

void LCD_Fill(u16 xsta, u16 ysta, u16 xend, u16 yend, u16 color)
{
    u16 i;
    u16 j;

    LCD_Address_Set(xsta, ysta, xend - 1, yend - 1);
    for (i = ysta; i < yend; i++)
    {
        for (j = xsta; j < xend; j++)
        {
            LCD_WR_DATA(color);
        }
    }
}

void LCD_DrawPoint(u16 x, u16 y, u16 color)
{
    LCD_Address_Set(x, y, x, y);
    LCD_WR_DATA(color);
}

void LCD_DrawLine(u16 x1, u16 y1, u16 x2, u16 y2, u16 color)
{
    u16 t;
    int xerr = 0;
    int yerr = 0;
    int delta_x;
    int delta_y;
    int distance;
    int incx;
    int incy;
    int uRow;
    int uCol;

    delta_x = x2 - x1;
    delta_y = y2 - y1;
    uRow = x1;
    uCol = y1;
    if (delta_x > 0)
    {
        incx = 1;
    }
    else if (delta_x == 0)
    {
        incx = 0;
    }
    else
    {
        incx = -1;
        delta_x = -delta_x;
    }
    if (delta_y > 0)
    {
        incy = 1;
    }
    else if (delta_y == 0)
    {
        incy = 0;
    }
    else
    {
        incy = -1;
        delta_y = -delta_y;
    }
    if (delta_x > delta_y)
    {
        distance = delta_x;
    }
    else
    {
        distance = delta_y;
    }
    for (t = 0; t < (u16)(distance + 1); t++)
    {
        LCD_DrawPoint((u16)uRow, (u16)uCol, color);
        xerr += delta_x;
        yerr += delta_y;
        if (xerr > distance)
        {
            xerr -= distance;
            uRow += incx;
        }
        if (yerr > distance)
        {
            yerr -= distance;
            uCol += incy;
        }
    }
}

void LCD_DrawRectangle(u16 x1, u16 y1, u16 x2, u16 y2, u16 color)
{
    LCD_DrawLine(x1, y1, x2, y1, color);
    LCD_DrawLine(x1, y1, x1, y2, color);
    LCD_DrawLine(x1, y2, x2, y2, color);
    LCD_DrawLine(x2, y1, x2, y2, color);
}

void Draw_Circle(u16 x0, u16 y0, u8 r, u16 color)
{
    int a = 0;
    int b = r;

    while (a <= b)
    {
        LCD_DrawPoint(x0 - (u16)b, y0 - (u16)a, color);
        LCD_DrawPoint(x0 + (u16)b, y0 - (u16)a, color);
        LCD_DrawPoint(x0 - (u16)a, y0 + (u16)b, color);
        LCD_DrawPoint(x0 - (u16)a, y0 - (u16)b, color);
        LCD_DrawPoint(x0 + (u16)b, y0 + (u16)a, color);
        LCD_DrawPoint(x0 + (u16)a, y0 - (u16)b, color);
        LCD_DrawPoint(x0 + (u16)a, y0 + (u16)b, color);
        LCD_DrawPoint(x0 - (u16)b, y0 + (u16)a, color);
        a++;
        if ((a * a + b * b) > (r * r))
        {
            b--;
        }
    }
}

void LCD_ShowChinese(u16 x, u16 y, u8 *s, u16 fc, u16 bc, u8 sizey, u8 mode)
{
    while (*s != 0)
    {
        if (sizey == 12)
        {
            LCD_ShowChinese12x12(x, y, s, fc, bc, sizey, mode);
        }
        else if (sizey == 16)
        {
            LCD_ShowChinese16x16(x, y, s, fc, bc, sizey, mode);
        }
        else if (sizey == 24)
        {
            LCD_ShowChinese24x24(x, y, s, fc, bc, sizey, mode);
        }
        else if (sizey == 32)
        {
            LCD_ShowChinese32x32(x, y, s, fc, bc, sizey, mode);
        }
        else
        {
            return;
        }
        s += 2;
        x += sizey;
    }
}

void LCD_ShowChinese12x12(u16 x, u16 y, u8 *s, u16 fc, u16 bc, u8 sizey, u8 mode)
{
    u8 i;
    u8 j;
    u8 m = 0;
    u16 k;
    u16 HZnum;
    u16 TypefaceNum;
    u16 x0 = x;

    TypefaceNum = (sizey / 8 + ((sizey % 8) ? 1U : 0U)) * sizey;
    HZnum = sizeof(tfont12) / sizeof(typFNT_GB12);
    for (k = 0; k < HZnum; k++)
    {
        if ((tfont12[k].Index[0] == *s) && (tfont12[k].Index[1] == *(s + 1)))
        {
            LCD_Address_Set(x, y, x + sizey - 1, y + sizey - 1);
            for (i = 0; i < TypefaceNum; i++)
            {
                for (j = 0; j < 8; j++)
                {
                    if (!mode)
                    {
                        if (tfont12[k].Msk[i] & (0x01U << j))
                        {
                            LCD_WR_DATA(fc);
                        }
                        else
                        {
                            LCD_WR_DATA(bc);
                        }
                        m++;
                        if (m % sizey == 0U)
                        {
                            m = 0;
                            break;
                        }
                    }
                    else
                    {
                        if (tfont12[k].Msk[i] & (0x01U << j))
                        {
                            LCD_DrawPoint(x, y, fc);
                        }
                        x++;
                        if ((x - x0) == sizey)
                        {
                            x = x0;
                            y++;
                            break;
                        }
                    }
                }
            }
        }
    }
}

void LCD_ShowChinese16x16(u16 x, u16 y, u8 *s, u16 fc, u16 bc, u8 sizey, u8 mode)
{
    u8 i;
    u8 j;
    u8 m = 0;
    u16 k;
    u16 HZnum;
    u16 TypefaceNum;
    u16 x0 = x;

    TypefaceNum = (sizey / 8 + ((sizey % 8) ? 1U : 0U)) * sizey;
    HZnum = sizeof(tfont16) / sizeof(typFNT_GB16);
    for (k = 0; k < HZnum; k++)
    {
        if ((tfont16[k].Index[0] == *s) && (tfont16[k].Index[1] == *(s + 1)))
        {
            LCD_Address_Set(x, y, x + sizey - 1, y + sizey - 1);
            for (i = 0; i < TypefaceNum; i++)
            {
                for (j = 0; j < 8; j++)
                {
                    if (!mode)
                    {
                        if (tfont16[k].Msk[i] & (0x01U << j))
                        {
                            LCD_WR_DATA(fc);
                        }
                        else
                        {
                            LCD_WR_DATA(bc);
                        }
                        m++;
                        if (m % sizey == 0U)
                        {
                            m = 0;
                            break;
                        }
                    }
                    else
                    {
                        if (tfont16[k].Msk[i] & (0x01U << j))
                        {
                            LCD_DrawPoint(x, y, fc);
                        }
                        x++;
                        if ((x - x0) == sizey)
                        {
                            x = x0;
                            y++;
                            break;
                        }
                    }
                }
            }
        }
    }
}

void LCD_ShowChinese24x24(u16 x, u16 y, u8 *s, u16 fc, u16 bc, u8 sizey, u8 mode)
{
    u8 i;
    u8 j;
    u8 m = 0;
    u16 k;
    u16 HZnum;
    u16 TypefaceNum;
    u16 x0 = x;

    TypefaceNum = (sizey / 8 + ((sizey % 8) ? 1U : 0U)) * sizey;
    HZnum = sizeof(tfont24) / sizeof(typFNT_GB24);
    for (k = 0; k < HZnum; k++)
    {
        if ((tfont24[k].Index[0] == *s) && (tfont24[k].Index[1] == *(s + 1)))
        {
            LCD_Address_Set(x, y, x + sizey - 1, y + sizey - 1);
            for (i = 0; i < TypefaceNum; i++)
            {
                for (j = 0; j < 8; j++)
                {
                    if (!mode)
                    {
                        if (tfont24[k].Msk[i] & (0x01U << j))
                        {
                            LCD_WR_DATA(fc);
                        }
                        else
                        {
                            LCD_WR_DATA(bc);
                        }
                        m++;
                        if (m % sizey == 0U)
                        {
                            m = 0;
                            break;
                        }
                    }
                    else
                    {
                        if (tfont24[k].Msk[i] & (0x01U << j))
                        {
                            LCD_DrawPoint(x, y, fc);
                        }
                        x++;
                        if ((x - x0) == sizey)
                        {
                            x = x0;
                            y++;
                            break;
                        }
                    }
                }
            }
        }
    }
}

void LCD_ShowChinese32x32(u16 x, u16 y, u8 *s, u16 fc, u16 bc, u8 sizey, u8 mode)
{
    u8 i;
    u8 j;
    u8 m = 0;
    u16 k;
    u16 HZnum;
    u16 TypefaceNum;
    u16 x0 = x;

    TypefaceNum = (sizey / 8 + ((sizey % 8) ? 1U : 0U)) * sizey;
    HZnum = sizeof(tfont32) / sizeof(typFNT_GB32);
    for (k = 0; k < HZnum; k++)
    {
        if ((tfont32[k].Index[0] == *s) && (tfont32[k].Index[1] == *(s + 1)))
        {
            LCD_Address_Set(x, y, x + sizey - 1, y + sizey - 1);
            for (i = 0; i < TypefaceNum; i++)
            {
                for (j = 0; j < 8; j++)
                {
                    if (!mode)
                    {
                        if (tfont32[k].Msk[i] & (0x01U << j))
                        {
                            LCD_WR_DATA(fc);
                        }
                        else
                        {
                            LCD_WR_DATA(bc);
                        }
                        m++;
                        if (m % sizey == 0U)
                        {
                            m = 0;
                            break;
                        }
                    }
                    else
                    {
                        if (tfont32[k].Msk[i] & (0x01U << j))
                        {
                            LCD_DrawPoint(x, y, fc);
                        }
                        x++;
                        if ((x - x0) == sizey)
                        {
                            x = x0;
                            y++;
                            break;
                        }
                    }
                }
            }
        }
    }
}

void LCD_ShowChar(u16 x, u16 y, u8 num, u16 fc, u16 bc, u8 sizey, u8 mode)
{
    u8 temp;
    u8 sizex;
    u8 t;
    u8 m = 0;
    u16 i;
    u16 TypefaceNum;
    u16 x0 = x;

    sizex = sizey / 2U;
    TypefaceNum = (sizex / 8U + ((sizex % 8U) ? 1U : 0U)) * sizey;
    num = num - ' ';
    LCD_Address_Set(x, y, x + sizex - 1U, y + sizey - 1U);
    for (i = 0; i < TypefaceNum; i++)
    {
        if (sizey == 12U)
        {
            temp = ascii_1206[num][i];
        }
        else if (sizey == 16U)
        {
            temp = ascii_1608[num][i];
        }
        else if (sizey == 24U)
        {
            temp = ascii_2412[num][i];
        }
        else if (sizey == 32U)
        {
            temp = ascii_3216[num][i];
        }
        else
        {
            return;
        }
        for (t = 0; t < 8U; t++)
        {
            if (!mode)
            {
                if (temp & (0x01U << t))
                {
                    LCD_WR_DATA(fc);
                }
                else
                {
                    LCD_WR_DATA(bc);
                }
                m++;
                if (m % sizex == 0U)
                {
                    m = 0;
                    break;
                }
            }
            else
            {
                if (temp & (0x01U << t))
                {
                    LCD_DrawPoint(x, y, fc);
                }
                x++;
                if ((x - x0) == sizex)
                {
                    x = x0;
                    y++;
                    break;
                }
            }
        }
    }
}

void LCD_ShowString(u16 x, u16 y, const u8 *p, u16 fc, u16 bc, u8 sizey, u8 mode)
{
    while (*p != '\0')
    {
        LCD_ShowChar(x, y, *p, fc, bc, sizey, mode);
        x += sizey / 2U;
        p++;
    }
}

u32 mypow(u8 m, u8 n)
{
    u32 result = 1U;

    while (n--)
    {
        result *= m;
    }
    return result;
}

void LCD_ShowIntNum(u16 x, u16 y, u16 num, u8 len, u16 fc, u16 bc, u8 sizey)
{
    u8 t;
    u8 temp;
    u8 enshow = 0;
    u8 sizex = sizey / 2U;

    for (t = 0; t < len; t++)
    {
        temp = (u8)((num / mypow(10U, len - t - 1U)) % 10U);
        if ((enshow == 0U) && (t < (u8)(len - 1U)))
        {
            if (temp == 0U)
            {
                LCD_ShowChar(x + t * sizex, y, ' ', fc, bc, sizey, 0);
                continue;
            }
            else
            {
                enshow = 1U;
            }
        }
        LCD_ShowChar(x + t * sizex, y, (u8)(temp + 48U), fc, bc, sizey, 0);
    }
}

void LCD_ShowFloatNum1(u16 x, u16 y, float num, u8 len, u16 fc, u16 bc, u8 sizey)
{
    u8 t;
    u8 temp;
    u8 sizex;
    u16 num1;

    sizex = sizey / 2U;
    num1 = (u16)(num * 100.0f);
    for (t = 0; t < len; t++)
    {
        temp = (u8)((num1 / mypow(10U, len - t - 1U)) % 10U);
        if (t == (u8)(len - 2U))
        {
            LCD_ShowChar(x + (len - 2U) * sizex, y, '.', fc, bc, sizey, 0);
            t++;
            len += 1U;
        }
        LCD_ShowChar(x + t * sizex, y, (u8)(temp + 48U), fc, bc, sizey, 0);
    }
}

void LCD_ShowPicture(u16 x, u16 y, u16 length, u16 width, const u8 pic[])
{
    u16 i;
    u16 j;
    u32 k = 0U;

    LCD_Address_Set(x, y, x + length - 1U, y + width - 1U);
    for (i = 0; i < length; i++)
    {
        for (j = 0; j < width; j++)
        {
            LCD_WR_DATA8(pic[k * 2U]);
            LCD_WR_DATA8(pic[k * 2U + 1U]);
            k++;
        }
    }
}