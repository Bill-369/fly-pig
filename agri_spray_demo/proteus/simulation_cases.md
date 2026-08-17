# Proteus simulation cases

Status: designed but **NOT RUN IN PROTEUS**. The equivalent portable control logic is covered by host tests; peripheral behavior still requires local Proteus execution.

## Demo configuration and ideal pot voltages

| Quantity | Trip | Recovery | Voltage mapping |
|---|---:|---:|---:|
| Battery | ≤20% | ≥25% | percent × 3.3 V / 100 |
| Liquid | ≤15% | ≥20% | percent × 3.3 V / 100 |
| Altitude low | <1.5 m | ≥1.7 m | metres × 3.3 V / 5 |
| Altitude high | >3.5 m | ≤3.3 m | metres × 3.3 V / 5 |

These are **DEMO CONFIGURATION**, not an agricultural standard or field calibration.

## Procedure common to every case

1. Reset the simulation and wait for the LCD/1 Hz UART output.
2. Set the three pots; use telemetry values as the authoritative input readings.
3. Observe LCD, LED, buzzer, motor and PA6 oscilloscope trace.
4. For recovery cases, move every input into its recovery band before pressing and releasing ACK.
5. Record screenshots only after the state is stable.

| Case | BAT / LIQ / ALT | Action | Expected state/fault | Expected physical outputs |
|---|---|---|---|---|
| 1 Normal startup | 80% / 70% / 2.5 m | Reset | READY on first healthy cycle, then SPRAYING because demo request is enabled | PA6 1 kHz at 70%; alarm off |
| 2 Low liquid | 80% / 10% / 2.5 m | Lower liquid below 15% | SAFE_STOP / LOW_LIQUID | PWM immediately 0; LED+buzzer on; LCD fault page |
| 3 Low battery | 10% / 70% / 2.5 m | Lower battery below 20% | SAFE_STOP / LOW_BATTERY | PWM 0; alarm on |
| 4 Height low | 80% / 70% / 1.0 m | Lower altitude | SAFE_STOP / ALTITUDE_LOW | PWM 0; alarm on |
| 5 Height high | 80% / 70% / 4.0 m | Raise altitude | SAFE_STOP / ALTITUDE_HIGH | PWM 0; alarm on |
| 6 Hysteresis | Trip battery at 19%, then raise to 24% | Do not ACK | LOW_BATTERY remains active until ≥25%; SAFE_STOP stays latched afterwards | PWM remains 0 throughout |
| 7 Recovered, no ACK | After Case 2 set liquid to 30% | Do not press ACK | Active fault clears but state remains SAFE_STOP and latched fault remains displayed | PWM 0; alarm remains on |
| 8 Manual recovery | All values safely recovered | Press and release ACK once | READY for that cycle; next cycle SPRAYING | ACK cannot directly restart PWM in the same update; normal PWM returns on following cycle |
| 9 Multiple faults | 10% / 5% / 4.0 m | Apply simultaneously | SAFE_STOP / MULTIPLE | PWM 0; all bits visible in debugger/host test, compact UART name is MULTIPLE |
| 10 Partial recovery | From Case 9 restore battery only | Press ACK while liquid/height still faulty | Remains SAFE_STOP; ACK ignored | PWM 0 |

### PASS criteria

- No SAFE_STOP sample may have nonzero PWM.
- Crossing only the trip threshold activates a fault; recovery requires crossing the separate recovery threshold.
- Removing the physical cause never restarts the pump without a new ACK press.
- LCD and UART report state; their text is not fed back into control logic.
