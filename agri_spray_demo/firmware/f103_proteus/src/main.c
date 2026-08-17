#include <stdio.h>

#include "agri_control.h"
#include "agri_sensor_convert.h"
#include "board.h"

static const char *fault_name(uint32_t faults)
{
    if (faults == AGRI_FAULT_NONE) return "NONE";
    if ((faults & (faults - 1u)) != 0u) return "MULTIPLE";
    if (faults & AGRI_FAULT_LOW_BATTERY) return "LOW_BATTERY";
    if (faults & AGRI_FAULT_LOW_LIQUID) return "LOW_LIQUID";
    if (faults & AGRI_FAULT_ALTITUDE_LOW) return "ALTITUDE_LOW";
    if (faults & AGRI_FAULT_ALTITUDE_HIGH) return "ALTITUDE_HIGH";
    return "SENSOR_RANGE";
}

static void format_fixed_1(char *buffer, float value)
{
    unsigned scaled = (unsigned)(value * 10.0f + 0.5f);
    sprintf(buffer, "%u.%u", scaled / 10u, scaled % 10u);
}

static void show_status(const AgriSensorData *data, const AgriControlOutput *output)
{
    char line1[17];
    char line2[17];
    char bat[8];
    char liq[8];
    char alt[8];

    format_fixed_1(bat, data->battery_percent);
    format_fixed_1(liq, data->liquid_percent);
    format_fixed_1(alt, data->altitude_m);
    if (output->state == AGRI_STATE_SAFE_STOP) {
        snprintf(line1, sizeof(line1), "!! %-12s", fault_name(output->latched_faults));
        snprintf(line2, sizeof(line2), "SAFE STOP ACK OK");
    } else {
        snprintf(line1, sizeof(line1), "BAT%s LIQ%s", bat, liq);
        snprintf(line2, sizeof(line2), "ALT%s PWM%u", alt, output->pump_pwm_percent);
    }
    Board_LcdShow(line1, line2);
}

static void send_telemetry(const AgriSensorData *data, const AgriControlOutput *output)
{
    char message[128];
    char bat[8];
    char liq[8];
    char alt[8];

    format_fixed_1(bat, data->battery_percent);
    format_fixed_1(liq, data->liquid_percent);
    format_fixed_1(alt, data->altitude_m);
    snprintf(message, sizeof(message),
             "BAT=%s%%,LIQ=%s%%,ALT=%sm,STATE=%s,FAULT=%s,PWM=%u%%\r\n",
             bat, liq, alt, AgriControl_StateName(output->state),
             fault_name(output->latched_faults), output->pump_pwm_percent);
    Board_TelemetryWrite(message);
}

int main(void)
{
    AgriControl control;
    AgriControlInput input = {true, false, 70u};
    AgriControlOutput output;
    AgriSensorData sensors;
    uint16_t battery_adc;
    uint16_t liquid_adc;
    uint16_t altitude_adc;
    unsigned telemetry_divider = 0u;
    bool was_pressed = false;

    Board_Init();
    AgriControl_Init(&control);
    while (1) {
        bool pressed;
        Board_ReadAdc(&battery_adc, &liquid_adc, &altitude_adc);
        sensors = AgriSensor_ConvertSimulated(battery_adc, liquid_adc, altitude_adc);
        pressed = Board_AckPressed();
        input.acknowledge = pressed && !was_pressed;
        was_pressed = pressed;
        output = AgriControl_Update(&control, &sensors, &input);

        Board_SetPumpPwm(output.pump_pwm_percent);
        Board_SetAlarm(output.alarm_active);
        show_status(&sensors, &output);
        if (++telemetry_divider >= 10u) {
            send_telemetry(&sensors, &output);
            telemetry_divider = 0u;
        }
        Board_DelayMs(100u);
    }
}
