# Keil 5 project creation checklist

No `.uvprojx`, startup file or HEX is fabricated here. On a Windows machine with Keil MDK 5:

1. Install the STM32F1 device pack and obtain ST's STM32F10x Standard Peripheral Library.
2. Create a new project for `STM32F103C8`, select the official startup file for the medium-density device, and enable `USE_STDPERIPH_DRIVER` plus `STM32F10X_MD`.
3. Add CMSIS `system_stm32f10x.c`; SPL sources for RCC, GPIO, ADC, TIM and USART; the three files under `../src`; and `../../common/agri_control.c` plus `agri_sensor_convert.c`.
4. Add include paths for CMSIS, SPL `inc`, `../inc`, and `../../common`.
5. Configure an 8 MHz crystal and 72 MHz system clock using the official system file. Enable "Create HEX File".
6. Build and retain the complete build log. Only after a zero-error build may the generated HEX be loaded into Proteus.
7. In Proteus set the MCU clock property to the same clock expected by the firmware, then execute all cases in `../../../proteus/simulation_cases.md`.

Current status: **NOT VERIFIED IN KEIL**. `snprintf` availability/code size depends on the selected C library; verify LCD and telemetry formatting in the local build.
