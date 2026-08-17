#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include "agri_config.h"
#include "agri_control.h"
#include "agri_sensor_convert.h"

static unsigned tests_run;
static unsigned tests_failed;
#define CHECK(c) do { if (!(c)) { printf("  FAIL %s:%d: %s\n", __FILE__, __LINE__, #c); return false; } } while (0)
#define RUN(t) do { bool p; ++tests_run; p = t(); printf("[%s] %s\n", p ? "PASS" : "FAIL", #t); if (!p) ++tests_failed; } while (0)

static const AgriSensorData normal = {80.0f, 70.0f, 2.5f};
static const AgriControlInput idle = {false, false, 70u};
static const AgriControlInput spray = {true, false, 70u};
static AgriControlOutput update(AgriControl *c, AgriSensorData s, AgriControlInput i) { return AgriControl_Update(c, &s, &i); }

static bool test_normal_startup(void) { AgriControl c; AgriControlOutput o; AgriControl_Init(&c); o=update(&c,normal,idle); CHECK(o.state==AGRI_STATE_READY); CHECK(o.active_faults==0u); CHECK(o.pump_pwm_percent==0u); return true; }
static bool test_normal_spraying(void) { AgriControl c; AgriControlOutput o; AgriControl_Init(&c); update(&c,normal,idle); o=update(&c,normal,spray); CHECK(o.state==AGRI_STATE_SPRAYING); CHECK(o.pump_pwm_percent==70u); CHECK(!o.alarm_active); return true; }
static bool test_low_battery(void) { AgriControl c; AgriSensorData d=normal; AgriControlOutput o; AgriControl_Init(&c); d.battery_percent=19.9f; o=update(&c,d,spray); CHECK(o.state==AGRI_STATE_SAFE_STOP); CHECK(o.active_faults&AGRI_FAULT_LOW_BATTERY); CHECK(o.pump_pwm_percent==0u); return true; }
static bool test_low_liquid(void) { AgriControl c; AgriSensorData d=normal; AgriControlOutput o; AgriControl_Init(&c); d.liquid_percent=14.9f; o=update(&c,d,spray); CHECK(o.active_faults&AGRI_FAULT_LOW_LIQUID); CHECK(o.pump_pwm_percent==0u); return true; }
static bool test_altitude_too_low(void) { AgriControl c; AgriSensorData d=normal; AgriControlOutput o; AgriControl_Init(&c); d.altitude_m=1.49f; o=update(&c,d,spray); CHECK(o.active_faults&AGRI_FAULT_ALTITUDE_LOW); CHECK(o.pump_pwm_percent==0u); return true; }
static bool test_altitude_too_high(void) { AgriControl c; AgriSensorData d=normal; AgriControlOutput o; AgriControl_Init(&c); d.altitude_m=3.51f; o=update(&c,d,spray); CHECK(o.active_faults&AGRI_FAULT_ALTITUDE_HIGH); CHECK(o.pump_pwm_percent==0u); return true; }
static bool test_hysteresis_near_thresholds(void) { AgriControl c; AgriSensorData d=normal; AgriControlOutput o; AgriControl_Init(&c); d.battery_percent=19.0f; update(&c,d,spray); d.battery_percent=24.9f; o=update(&c,d,idle); CHECK(o.active_faults&AGRI_FAULT_LOW_BATTERY); d.battery_percent=25.0f; o=update(&c,d,idle); CHECK(!(o.active_faults&AGRI_FAULT_LOW_BATTERY)); CHECK(o.state==AGRI_STATE_SAFE_STOP); return true; }
static bool test_recovery_without_ack_stays_stopped(void) { AgriControl c; AgriSensorData d=normal; AgriControlOutput o; AgriControl_Init(&c); d.liquid_percent=10.0f; update(&c,d,spray); d.liquid_percent=30.0f; o=update(&c,d,spray); CHECK(o.active_faults==0u); CHECK(o.state==AGRI_STATE_SAFE_STOP); CHECK(o.pump_pwm_percent==0u); return true; }
static bool test_ack_recovers_to_ready(void) { AgriControl c; AgriSensorData d=normal; AgriControlInput ack={true,true,70u}; AgriControlOutput o; AgriControl_Init(&c); d.liquid_percent=10.0f; update(&c,d,spray); o=update(&c,normal,ack); CHECK(o.state==AGRI_STATE_READY); CHECK(o.latched_faults==0u); CHECK(o.pump_pwm_percent==0u); return true; }
static bool test_multiple_simultaneous_faults(void) { AgriControl c; AgriSensorData d={10.0f,5.0f,4.0f}; AgriControlOutput o; AgriControl_Init(&c); o=update(&c,d,spray); CHECK(o.active_faults&AGRI_FAULT_LOW_BATTERY); CHECK(o.active_faults&AGRI_FAULT_LOW_LIQUID); CHECK(o.active_faults&AGRI_FAULT_ALTITUDE_HIGH); CHECK(o.pump_pwm_percent==0u); return true; }
static bool test_partial_multi_fault_recovery(void) { AgriControl c; AgriSensorData d={10.0f,5.0f,2.5f}; AgriControlInput ack={false,true,70u}; AgriControlOutput o; AgriControl_Init(&c); update(&c,d,spray); d.battery_percent=30.0f; o=update(&c,d,ack); CHECK(!(o.active_faults&AGRI_FAULT_LOW_BATTERY)); CHECK(o.active_faults&AGRI_FAULT_LOW_LIQUID); CHECK(o.state==AGRI_STATE_SAFE_STOP); return true; }
static bool test_safe_stop_always_forces_zero_pwm(void) { AgriControl c; AgriSensorData d=normal; AgriControlInput max={true,false,255u}; AgriControlOutput o; AgriControl_Init(&c); d.altitude_m=4.5f; o=update(&c,d,max); CHECK(o.state==AGRI_STATE_SAFE_STOP); CHECK(o.pump_pwm_percent==0u); o=update(&c,normal,max); CHECK(o.state==AGRI_STATE_SAFE_STOP); CHECK(o.pump_pwm_percent==0u); return true; }
static bool test_simulated_adc_conversion(void) { AgriSensorData d=AgriSensor_ConvertSimulated(4095u,2048u,2048u); CHECK(fabsf(d.battery_percent-100.0f)<0.001f); CHECK(fabsf(d.liquid_percent-50.012f)<0.02f); CHECK(fabsf(d.altitude_m-2.5006f)<0.002f); return true; }

int main(void) { RUN(test_normal_startup); RUN(test_normal_spraying); RUN(test_low_battery); RUN(test_low_liquid); RUN(test_altitude_too_low); RUN(test_altitude_too_high); RUN(test_hysteresis_near_thresholds); RUN(test_recovery_without_ack_stays_stopped); RUN(test_ack_recovers_to_ready); RUN(test_multiple_simultaneous_faults); RUN(test_partial_multi_fault_recovery); RUN(test_safe_stop_always_forces_zero_pwm); RUN(test_simulated_adc_conversion); printf("\n%u tests, %u failures\n",tests_run,tests_failed); return tests_failed==0u?EXIT_SUCCESS:EXIT_FAILURE; }
