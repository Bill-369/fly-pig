# Phase 2 verification record

## Scope labels

- **HOST VERIFIED:** portable C conversion and control behavior compiled and executed on Linux with GCC.
- **STATICALLY REVIEWED:** pin use and F103 SPL skeleton inspected against the original repository and bundled ST device support, but not executed on target.
- **NOT VERIFIED IN KEIL:** no µVision/Arm Compiler or STM32F10x vendor library exists in this environment.
- **NOT VERIFIED IN PROTEUS:** Proteus is not installed; no `.pdsprj` or simulation result is claimed.
- **NOT HARDWARE VERIFIED:** no STM32 board, analog sensors, pump or driver stage was measured.

## Host test command and result

Command:

```sh
make -C agri_spray_demo/firmware/tests clean test
```

Compiler flags are `-std=c11 -Wall -Wextra -Werror -pedantic -O2`. On 2026-08-17 the command completed successfully with **13 tests, 0 failures**:

1. normal startup → READY;
2. normal spray → SPRAYING and 70% PWM;
3. low battery trip;
4. low liquid trip;
5. altitude-low trip;
6. altitude-high trip;
7. trip/recovery hysteresis boundary;
8. recovered input without ACK stays stopped;
9. ACK after complete recovery returns READY with zero PWM in that update;
10. simultaneous battery/liquid/altitude faults;
11. partial multi-fault recovery ignores ACK;
12. SAFE_STOP always forces PWM to zero, including after sensor recovery;
13. 12-bit simulated ADC conversion endpoints/midpoints.

The test executable is generated under `firmware/tests/build/` and is not source material. It should be cleaned before committing.

## Static separation checks

The common core is required not to mention STM32 headers or peripheral registers. Verify with:

```sh
! rg -n 'stm32|GPIO|ADC[0-9]|TIM[0-9]|USART[0-9]' agri_spray_demo/firmware/common --glob '*.[ch]'
```

The F103 skeleton uses only the STM32F10x SPL API (`stm32f10x.h` plus RCC/GPIO/ADC/TIM/USART functions). It does not include HAL headers. Vendor library files are not copied into this repository.

## Original-project isolation check

After phase 2, every changed path must begin with `agri_spray_demo/`. Use:

```sh
test -z "$(git diff --name-only HEAD^ -- . ':(exclude)agri_spray_demo/**')"
```

This checks repository isolation, not electrical pin availability.

## Requirements traceability

| Requirement | Implementation evidence | Verification |
|---|---|---|
| Three ADC inputs | `agri_sensor_convert.*`, F103 PA0/1/2 board adapter | Host conversion test; target pending |
| Hysteresis | separate trip/recovery constants and retained active bits | Host boundary test passed |
| Fault latch + ACK | `AgriControl_Update` SAFE_STOP branch | Host recovery tests passed |
| PWM zero in SAFE_STOP | output assigned only for SPRAYING | Host invariant tests passed; waveform pending |
| LCD separation | LCD formatting is in F103 `main.c`, outside common core | Static review; display pending |
| UART telemetry | F103 `main.c` + USART1 board adapter | Static review; terminal pending |
| F401 isolation | separate pin audit; no F401 BSP added to original project | Git path check |
| F103 equivalence label | README, mapping and Proteus docs | Documentation review |

## Local verification still required

1. Install Keil MDK 5, STM32F1 DFP and official STM32F10x SPL; follow `firmware/f103_proteus/keil/README.md`, build, and save the full log and HEX.
2. Open Proteus with an STM32F103C8T6 VSM model, manually wire `proteus/connection_table.md`, load that verified HEX and run every simulation case.
3. Capture PA6 duty cycle and its transition to constant low during a fault; verify UART and LCD simultaneously.
4. Before any F401 wiring, check the actual fly-pig PCB/netlist or development-board header against the software candidates, then implement a separate F401 adapter without changing timer ownership.
5. Real hardware requires divider, protection, MOSFET/thermal and EMI calculations based on actual battery and pump specifications.
