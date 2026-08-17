# STM32F103C8T6 Proteus functional target

This is an explicitly labelled **functional-equivalence simulation target**. It exercises three simulated ADC inputs, the MCU-independent safety core, pump PWM, alarm GPIOs, LCD1602 and USART telemetry. It does not validate STM32F401 hardware, flight dynamics, GY-86 attitude, pump flow or agricultural effectiveness.

## Toolchain policy

The target consistently uses the STM32F10x Standard Peripheral Library (SPL); it does not mix HAL and register-level peripheral setup. Vendor startup, CMSIS and SPL sources are intentionally not copied into this repository. Obtain the official STM32F10x SPL package locally and add it to Keil 5 as described in `keil/README.md`.

## Modules

- `src/main.c`: 100 ms acquisition/control/output loop, LCD presentation and 1 Hz telemetry.
- `src/board.c`: PA0/1/2 ADC, PA6 TIM3_CH1 PWM, alarm/ACK GPIO and USART1.
- `src/lcd1602.c`: HD44780-compatible 4-bit driver.
- `../common`: portable conversion, hysteresis, fault latch and state machine.

All three potentiometers are **SIMULATED SENSOR INPUT** sources. Pump switching is a **DEMO DRIVER MODEL** and must use a transistor/MOSFET stage; PA6 must not directly drive a motor.

## Verification status

`NOT VERIFIED IN KEIL` and `NOT VERIFIED IN PROTEUS`. The current environment lacks both tools and does not provide STM32F10x SPL. The portable core is separately built and executed by the host tests.
