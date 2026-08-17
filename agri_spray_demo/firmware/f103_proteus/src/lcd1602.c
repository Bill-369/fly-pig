#include "lcd1602.h"
#include "board.h"
#include "stm32f10x.h"

/* PB8=RS, PB9=E, PB12..PB15=D4..D7; RW is tied to GND. */
#define LCD_RS GPIO_Pin_8
#define LCD_E  GPIO_Pin_9
#define LCD_DATA (GPIO_Pin_12 | GPIO_Pin_13 | GPIO_Pin_14 | GPIO_Pin_15)

static void pulse_enable(void)
{
    GPIO_SetBits(GPIOB, LCD_E);
    Board_DelayMs(1u);
    GPIO_ResetBits(GPIOB, LCD_E);
    Board_DelayMs(1u);
}

static void write_nibble(unsigned char value)
{
    GPIO_ResetBits(GPIOB, LCD_DATA);
    GPIO_SetBits(GPIOB, (uint16_t)(value & 0x0fu) << 12);
    pulse_enable();
}

static void write_byte(unsigned char value, int data)
{
    GPIO_WriteBit(GPIOB, LCD_RS, data ? Bit_SET : Bit_RESET);
    write_nibble((unsigned char)(value >> 4));
    write_nibble(value);
}

static void command(unsigned char value)
{
    write_byte(value, 0);
    Board_DelayMs(value == 0x01u ? 2u : 1u);
}

void Lcd1602_Init(void)
{
    GPIO_InitTypeDef gpio;
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);
    gpio.GPIO_Pin = LCD_RS | LCD_E | LCD_DATA;
    gpio.GPIO_Speed = GPIO_Speed_50MHz;
    gpio.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_Init(GPIOB, &gpio);
    GPIO_ResetBits(GPIOB, LCD_RS | LCD_E | LCD_DATA);
    Board_DelayMs(20u);
    write_nibble(0x03u); Board_DelayMs(5u);
    write_nibble(0x03u); Board_DelayMs(1u);
    write_nibble(0x03u);
    write_nibble(0x02u);
    command(0x28u); /* 4-bit, two-line, 5x8 */
    command(0x0cu); /* display on, cursor off */
    command(0x06u); /* increment */
    command(0x01u);
}

static void write_line(unsigned char address, const char *text)
{
    unsigned char i;
    command(address);
    for (i = 0u; i < 16u; ++i) {
        write_byte(text[i] == '\0' ? ' ' : (unsigned char)text[i], 1);
        if (text[i] == '\0') {
            ++i;
            while (i < 16u) { write_byte(' ', 1); ++i; }
            break;
        }
    }
}

void Lcd1602_WriteLines(const char *line1, const char *line2)
{
    write_line(0x80u, line1);
    write_line(0xc0u, line2);
}
