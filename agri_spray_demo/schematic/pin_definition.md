# Final pin definition and conflict audit

## Audit method and limits

The audit re-scanned every non-vendor `*.c`/`*.h` file under the original `BSP/`, `USER/`, `PID/`, AHRS and attitude directories. It also checked the bundled ST CMSIS/SPL definitions for the target and the Keil device selection `STM32F401RETx`. No original file is modified.

“Free” below means no use found in this repository's application source. It is not a PCB continuity test: the repository contains no board schematic, so a local board must still be checked before wiring. AF/channel assignments must be checked once more against the exact ST datasheet revision used for hardware manufacture.

## A. STM32F401RETx integration target

| Function | Pin | Peripheral / mode | Existing use | Conflict result and integration rule |
|---|---|---|---|---|
| Battery ADC | PC0 | ADC1_IN10, analog | None found | Free in source; candidate accepted |
| Liquid ADC | PC1 | ADC1_IN11, analog | None found | Free in source; candidate accepted |
| Altitude ADC | PC2 | ADC1_IN12, analog | None found | Free in source; candidate accepted |
| Pump PWM | PB8 | TIM10_CH1, AF3 | None found | Free pin and unused timer. TIM10 shares the `TIM1_UP_TIM10` vector, but the proposed PWM uses no update interrupt, so it does not disturb TIM1_CH4 PPM capture |
| Buzzer | PC4 | GPIO output | None found | Free in source; use a transistor driver |
| ACK button | PC5 | GPIO input/pull-up | None found | Free in source; new candidate added for manual recovery |
| Status LED | PA5 | GPIO output | Existing status LED | Intentional reuse, not a second owner; call the existing LED abstraction |
| Telemetry TX/RX | PC6/PC7 | USART6 AF8 | ESP8266/PID/telemetry | **Occupied but intentionally shared function.** Do not initialize it twice. A future F401 integration adapter must serialize spray telemetry through the existing USART6 service and consolidate the multiple IRQ owners first |
| Display | PB6/PB7 | I2C1 AF4 | Existing SSD1305 OLED | Intentional reuse of existing display bus/driver; do not attach the F103 LCD wiring to F401 |

### Existing resources explicitly avoided

- PA6/PA7/PB0/PB1 and TIM3 are the four ESC outputs, so the F401 pump does not use TIM3.
- PA11/TIM1_CH4 is actual PPM capture; PA9/PA10 are USART1.
- PA8/PC9 are I2C3 for GY-86.
- TIM2 and TIM5 provide AHRS/PID time bases; TIM4 provides delays.
- PC13 is used by the GY-86 calibration button, hence ACK uses PC5.

### F401 conclusion

PC0/PC1/PC2, PB8, PC4 and PC5 have no source-level conflict. PA5, PC6/PC7 and PB6/PB7 are deliberate reuse points and require a single owning driver. Because no PCB source is present, these remain **final software integration candidates**, not proven free pads on a manufactured board.

## B. STM32F103C8T6 Proteus functional target

| Function | Pin | Peripheral / mode | Notes |
|---|---|---|---|
| Battery potentiometer | PA0 | ADC1_IN0 | SIMULATED SENSOR INPUT, 0–3.3 V |
| Liquid potentiometer | PA1 | ADC1_IN1 | SIMULATED SENSOR INPUT, 0–3.3 V |
| Altitude potentiometer | PA2 | ADC1_IN2 | SIMULATED SENSOR INPUT, 0–3.3 V maps to 0–5.0 m |
| Pump PWM | PA6 | TIM3_CH1 | 1 kHz; MOSFET gate driver, never direct motor drive |
| Telemetry TX | PA9 | USART1_TX | 115200 8-N-1 to Virtual Terminal RXD |
| Telemetry RX | PA10 | USART1_RX | Reserved; may connect to Virtual Terminal TXD |
| Buzzer | PB0 | GPIO push-pull | Drives NPN base through 1 kΩ |
| Status LED | PB1 | GPIO push-pull | Active high through 330 Ω |
| LCD RS | PB8 | GPIO push-pull | HD44780 4-bit mode |
| LCD E | PB9 | GPIO push-pull | LCD enable |
| LCD D4..D7 | PB12..PB15 | GPIO push-pull | LCD data nibble |
| ACK button | PC13 | GPIO input pull-up | Active low; firmware detects a press edge |
| Crystal | PD0/PD1 | OSC_IN/OSC_OUT | 8 MHz crystal in the documented build route |
| Reset | NRST | Reset input | 10 kΩ pull-up and pushbutton to GND |
| Debug/program | PA13/PA14 | SWDIO/SWCLK | Keep accessible; not otherwise allocated |

This allocation avoids PA0/1/2 ADC versus PA6 PWM versus PA9/10 USART overlap. PB3/PB4 remain untouched to avoid JTAG/remap complications. The F103 target has no four-ESC outputs because it is not the flight controller.
