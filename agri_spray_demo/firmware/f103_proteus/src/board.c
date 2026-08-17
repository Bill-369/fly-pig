#include "board.h"
#include "lcd1602.h"
#include "stm32f10x.h"

static volatile uint32_t delay_ms;

void SysTick_Handler(void)
{
    if (delay_ms != 0u) {
        --delay_ms;
    }
}

void Board_DelayMs(uint32_t milliseconds)
{
    delay_ms = milliseconds;
    while (delay_ms != 0u) {
    }
}

static void gpio_init(void)
{
    GPIO_InitTypeDef gpio;
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_GPIOB |
                           RCC_APB2Periph_GPIOC | RCC_APB2Periph_AFIO, ENABLE);

    gpio.GPIO_Speed = GPIO_Speed_50MHz;
    gpio.GPIO_Mode = GPIO_Mode_AF_PP;
    gpio.GPIO_Pin = GPIO_Pin_6;              /* PA6 TIM3_CH1 pump PWM */
    GPIO_Init(GPIOA, &gpio);

    gpio.GPIO_Mode = GPIO_Mode_Out_PP;
    gpio.GPIO_Pin = GPIO_Pin_0 | GPIO_Pin_1; /* PB0 buzzer, PB1 LED */
    GPIO_Init(GPIOB, &gpio);
    GPIO_ResetBits(GPIOB, GPIO_Pin_0 | GPIO_Pin_1);

    gpio.GPIO_Mode = GPIO_Mode_IPU;
    gpio.GPIO_Pin = GPIO_Pin_13;             /* PC13 ACK, active low */
    GPIO_Init(GPIOC, &gpio);
}

static void adc_init(void)
{
    ADC_InitTypeDef adc;
    GPIO_InitTypeDef gpio;
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC1 | RCC_APB2Periph_GPIOA, ENABLE);
    RCC_ADCCLKConfig(RCC_PCLK2_Div6);         /* 72 MHz / 6 = 12 MHz */
    gpio.GPIO_Pin = GPIO_Pin_0 | GPIO_Pin_1 | GPIO_Pin_2;
    gpio.GPIO_Mode = GPIO_Mode_AIN;
    gpio.GPIO_Speed = GPIO_Speed_2MHz;
    GPIO_Init(GPIOA, &gpio);
    ADC_StructInit(&adc);
    adc.ADC_Mode = ADC_Mode_Independent;
    adc.ADC_ContinuousConvMode = DISABLE;
    adc.ADC_ExternalTrigConv = ADC_ExternalTrigConv_None;
    adc.ADC_DataAlign = ADC_DataAlign_Right;
    adc.ADC_NbrOfChannel = 1;
    ADC_Init(ADC1, &adc);
    ADC_Cmd(ADC1, ENABLE);
    ADC_ResetCalibration(ADC1);
    while (ADC_GetResetCalibrationStatus(ADC1) != RESET) {}
    ADC_StartCalibration(ADC1);
    while (ADC_GetCalibrationStatus(ADC1) != RESET) {}
}

static uint16_t adc_read(uint8_t channel)
{
    ADC_RegularChannelConfig(ADC1, channel, 1u, ADC_SampleTime_55Cycles5);
    ADC_SoftwareStartConvCmd(ADC1, ENABLE);
    while (ADC_GetFlagStatus(ADC1, ADC_FLAG_EOC) == RESET) {}
    return ADC_GetConversionValue(ADC1);
}

void Board_ReadAdc(uint16_t *battery, uint16_t *liquid, uint16_t *altitude)
{
    *battery = adc_read(ADC_Channel_0);
    *liquid = adc_read(ADC_Channel_1);
    *altitude = adc_read(ADC_Channel_2);
}

static void pwm_init(void)
{
    TIM_TimeBaseInitTypeDef timer;
    TIM_OCInitTypeDef output;
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM3, ENABLE);
    timer.TIM_Prescaler = 72u - 1u;           /* 1 MHz counter */
    timer.TIM_Period = 1000u - 1u;            /* 1 kHz PWM */
    timer.TIM_ClockDivision = TIM_CKD_DIV1;
    timer.TIM_CounterMode = TIM_CounterMode_Up;
    TIM_TimeBaseInit(TIM3, &timer);
    TIM_OCStructInit(&output);
    output.TIM_OCMode = TIM_OCMode_PWM1;
    output.TIM_OutputState = TIM_OutputState_Enable;
    output.TIM_OCPolarity = TIM_OCPolarity_High;
    output.TIM_Pulse = 0u;
    TIM_OC1Init(TIM3, &output);
    TIM_OC1PreloadConfig(TIM3, TIM_OCPreload_Enable);
    TIM_ARRPreloadConfig(TIM3, ENABLE);
    TIM_Cmd(TIM3, ENABLE);
}

void Board_SetPumpPwm(uint8_t percent)
{
    if (percent > 100u) percent = 100u;
    TIM_SetCompare1(TIM3, (uint16_t)percent * 10u);
}

static void usart_init(void)
{
    GPIO_InitTypeDef gpio;
    USART_InitTypeDef usart;
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1 | RCC_APB2Periph_GPIOA, ENABLE);
    gpio.GPIO_Speed = GPIO_Speed_50MHz;
    gpio.GPIO_Mode = GPIO_Mode_AF_PP;
    gpio.GPIO_Pin = GPIO_Pin_9;
    GPIO_Init(GPIOA, &gpio);
    gpio.GPIO_Mode = GPIO_Mode_IN_FLOATING;
    gpio.GPIO_Pin = GPIO_Pin_10;
    GPIO_Init(GPIOA, &gpio);
    USART_StructInit(&usart);
    usart.USART_BaudRate = 115200u;
    usart.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
    USART_Init(USART1, &usart);
    USART_Cmd(USART1, ENABLE);
}

void Board_TelemetryWrite(const char *text)
{
    while (*text != '\0') {
        while (USART_GetFlagStatus(USART1, USART_FLAG_TXE) == RESET) {}
        USART_SendData(USART1, (uint16_t)(uint8_t)*text++);
    }
}

void Board_SetAlarm(bool active)
{
    BitAction value = active ? Bit_SET : Bit_RESET;
    GPIO_WriteBit(GPIOB, GPIO_Pin_0, value);
    GPIO_WriteBit(GPIOB, GPIO_Pin_1, value);
}

bool Board_AckPressed(void)
{
    return GPIO_ReadInputDataBit(GPIOC, GPIO_Pin_13) == Bit_RESET;
}

void Board_LcdShow(const char *line1, const char *line2)
{
    Lcd1602_WriteLines(line1, line2);
}

void Board_Init(void)
{
    SystemInit();
    SysTick_Config(SystemCoreClock / 1000u);
    gpio_init();
    adc_init();
    pwm_init();
    usart_init();
    Lcd1602_Init();
    Board_SetPumpPwm(0u);
    Board_SetAlarm(false);
}
