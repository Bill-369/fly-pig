# STM32F401 ↔ STM32F103 functional mapping

The table maps **functions, not equal pins or binary compatibility**. F103 is a Proteus functional platform; F401 remains the product/integration target.

| Function | F401 integration target | F103 simulation target | What the simulation proves |
|---|---|---|---|
| Battery input | PC0 / ADC1_IN10 | PA0 / ADC1_IN0 | ADC count-to-percent and low-battery logic |
| Liquid input | PC1 / ADC1_IN11 | PA1 / ADC1_IN1 | ADC count-to-percent and low-liquid logic |
| Altitude input | PC2 / ADC1_IN12 | PA2 / ADC1_IN2 | ADC count-to-0–5 m and height interlocks |
| Pump PWM | PB8 / TIM10_CH1 | PA6 / TIM3_CH1 | Nonzero duty when spraying and immediate zero in SAFE_STOP |
| Telemetry | existing PC6/PC7 USART6 service | PA9/PA10 USART1 | Human-readable state/fault output |
| Display | existing PB6/PB7 I2C1 OLED | PB8/PB9/PB12..15 LCD1602 | Presentation only; text never drives control decisions |
| Alarm | PC4 GPIO + existing PA5 LED | PB0 buzzer + PB1 LED | SAFE_STOP indication |
| Manual ACK | PC5 GPIO | PC13 GPIO | Fault-clear acknowledgement edge |
| Flight context | GY-86, AHRS, PID, PPM, TIM3 ESC outputs | Not simulated | Nothing—the F103 demo does not claim flight validation |

Both targets consume the same files in `firmware/common/`. Only their board adapters differ. Thresholds, hysteresis and fault-latch semantics therefore remain testable independently of STM32 registers.
