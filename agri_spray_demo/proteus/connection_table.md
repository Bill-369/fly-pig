# Proteus manual connection table — STM32F103 functional demo

No `.pdsprj` is supplied because Proteus is unavailable in this environment. This table is the reproducible source of truth. Search terms vary slightly by Proteus release; verify the exact library model before placing it.

## 1. Parts to place

| Qty | Proteus search keyword | Purpose / nominal value |
|---:|---|---|
| 1 | `STM32F103C8T6` | LQFP48 MCU, program file set only after a real Keil build |
| 1 | `CRYSTAL` | 8 MHz |
| 2 | `CAP` | 22 pF crystal loads |
| 4 | `CAP` | 100 nF MCU decoupling (minimum schematic representation) |
| 2 | `CAP-ELEC` | 4.7 µF on 3.3 V; 100 µF near demo motor supply |
| 5 | `RES` | 10 kΩ: BOOT0, NRST, MOSFET gate, NPN base pulldown, LCD contrast pot may replace one |
| 1 | `POT-HG` / `POTENTIOMETER` | 10 kΩ LCD contrast |
| 3 | `POT-HG` / `POTENTIOMETER` | 10 kΩ simulated Battery/Liquid/Altitude inputs |
| 1 | `LM016L` | 16×2 HD44780-compatible LCD |
| 1 | `VIRTUAL TERMINAL` | USART display, 115200 8-N-1 |
| 1 | `BUTTON` / `SW-PB` | ACK, normally open |
| 1 | `BUTTON` / `SW-PB` | Reset, normally open |
| 1 | `LED-RED` | SAFE_STOP status |
| 1 | `RES` | 330 Ω LED limiter |
| 1 | `BUZZER` / `ACTIVE BUZZER` | Audible alarm |
| 1 | `2N2222` | Buzzer low-side driver |
| 1 | `RES` | 1 kΩ NPN base resistor |
| 1 | `MOTOR-DC` | Pump representation |
| 1 | `IRLZ44N` or logic-level `NMOS` | DEMO DRIVER MODEL |
| 1 | `RES` | 100 Ω MOSFET gate resistor |
| 1 | `1N4007` | Demo flyback diode |
| 1 | `OSCILLOSCOPE` or `LOGIC ANALYSER` | Observe PA6 PWM |
| as needed | `VCC`, `+3V3`, `+5V`, `GND`, `DC SOURCE` | MCU/LCD/motor supplies |

## 2. MCU power, boot, clock and reset

STM32F103C8 LQFP48 physical pin numbers are included to make the wiring unambiguous.

| MCU signal (package pin) | Connect to | Components / setting |
|---|---|---|
| VDD pins 24, 36, 48 | +3.3 V | One 100 nF to GND at each pin; add 4.7 µF bulk |
| VDDA pin 9 | +3.3 V | 100 nF to VSSA; keep analog wiring short |
| VSS pins 23, 35, 47 | GND | Common MCU/motor-driver reference |
| VSSA pin 8 | GND | Analog ground |
| VBAT pin 1 | +3.3 V | Demo has no backup battery |
| BOOT0 pin 44 | GND | 10 kΩ pulldown |
| OSC_IN/PD0 pin 5 | Crystal terminal 1 | 8 MHz; 22 pF from this node to GND |
| OSC_OUT/PD1 pin 6 | Crystal terminal 2 | 22 pF from this node to GND |
| NRST pin 7 | +3.3 V and reset button | 10 kΩ pull-up; normally-open button from NRST to GND |
| PA13 pin 34 / PA14 pin 37 | Optional SWD header | SWDIO/SWCLK; leave unconnected in simulation if not debugging |

Set the MCU clock/program properties consistently with the locally built firmware. Do not attach an unverified HEX.

## 3. Three simulated ADC inputs

For each 10 kΩ potentiometer: terminal A → +3.3 V, terminal B → GND, wiper → the listed ADC pin. These are **SIMULATED SENSOR INPUT**, not real sensors.

| Pot label | Wiper target | MCU package pin | Conversion |
|---|---|---:|---|
| BATTERY | PA0 / ADC1_IN0 | 10 | 0–3.3 V → 0–4095 → 0–100% |
| LIQUID | PA1 / ADC1_IN1 | 11 | 0–3.3 V → 0–4095 → 0–100% |
| ALTITUDE | PA2 / ADC1_IN2 | 12 | 0–3.3 V → 0–4095 → 0–5.0 m |

Useful ideal voltages: 80%=2.64 V, 70%=2.31 V, 20%=0.66 V, 15%=0.495 V, 2.5 m=1.65 V. Potentiometer UI precision may differ, so confirm actual ADC/telemetry values rather than relying only on knob position.

## 4. LCD1602 (`LM016L`) in 4-bit write-only mode

| LCD pin | Name | Connect to |
|---:|---|---|
| 1 | VSS | GND |
| 2 | VDD | +5 V for the common Proteus model; see logic-level note below |
| 3 | VEE/V0 | Wiper of 10 kΩ contrast pot; pot ends to +5 V and GND |
| 4 | RS | MCU PB8, package pin 45 |
| 5 | RW | GND (write-only) |
| 6 | E | MCU PB9, package pin 46 |
| 7–10 | D0–D3 | Leave unconnected |
| 11 | D4 | MCU PB12, package pin 25 |
| 12 | D5 | MCU PB13, package pin 26 |
| 13 | D6 | MCU PB14, package pin 27 |
| 14 | D7 | MCU PB15, package pin 28 |
| 15/16 | Backlight A/K, if model exposes them | A to +5 V through model-appropriate resistor; K to GND |

RW is grounded, so the LCD never drives 5 V back into STM32 pins. If the chosen model does not recognize 3.3 V logic-high, use a 3.3 V-compatible LCD model or a level shifter; do not quietly assume electrical compatibility for real hardware.

## 5. USART Virtual Terminal

| Source | Destination | Setting |
|---|---|---|
| PA9 / USART1_TX, MCU pin 30 | Virtual Terminal `RXD` | 115200 baud, 8 data, no parity, 1 stop |
| PA10 / USART1_RX, MCU pin 31 | Virtual Terminal `TXD` | Optional/reserved; firmware currently does not consume commands |
| GND | Terminal reference if exposed | Common GND |

Expected line form: `BAT=78.0%,LIQ=65.0%,ALT=2.3m,STATE=SPRAYING,FAULT=NONE,PWM=70%`.

## 6. Pump PWM and demo motor driver

| From | Through | To |
|---|---|---|
| PA6 / TIM3_CH1, MCU pin 16 | 100 Ω | NMOS gate |
| NMOS gate | 10 kΩ | GND |
| NMOS source | direct | GND |
| NMOS drain | direct | DC motor negative |
| DC motor positive | direct | +6 V demo motor supply (or motor model rating) |
| 1N4007 anode | direct | NMOS drain / motor negative |
| 1N4007 cathode | direct | motor positive |
| Motor supply negative | direct | Common GND |
| PA6 | oscilloscope CH-A / logic analyser input | Observe 1 kHz, nominal 70% PWM |

Add 100 µF across the demo motor supply. This is a **DEMO DRIVER MODEL**, not a production pump power stage. PA6 must never connect directly to the motor.

## 7. Alarm LED, buzzer and ACK

| Function | Wiring |
|---|---|
| Status LED | PB1 (MCU pin 19) → 330 Ω → LED anode; LED cathode → GND. Active high |
| Buzzer driver | PB0 (pin 18) → 1 kΩ → 2N2222 base; 10 kΩ base-to-GND; emitter → GND; collector → buzzer negative; buzzer positive → its rated supply |
| ACK | PC13 (pin 2) → normally-open pushbutton → GND. Firmware enables pull-up and recognizes a new press edge. An external 10 kΩ to +3.3 V may be added for visible schematic intent |

## 8. Pre-power checklist

1. No 5 V or motor voltage reaches a GPIO or ADC pin.
2. All MCU supply pins and grounds are connected.
3. BOOT0 is low; NRST is high when buttons are released.
4. Pot wipers stay within 0–3.3 V.
5. LCD RW is grounded and contrast pot is connected.
6. Motor uses the MOSFET and flyback diode; grounds are common.
7. Virtual Terminal matches 115200 8-N-1.
8. The loaded HEX was produced by a real local Keil build for STM32F103C8—not the F401 project.
