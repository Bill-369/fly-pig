#include "agri_control.h"
#include "agri_config.h"

static bool sensor_data_valid(const AgriSensorData *sensors)
{
    return sensors != 0 &&
           sensors->battery_percent >= AGRI_SENSOR_PERCENT_MIN &&
           sensors->battery_percent <= AGRI_SENSOR_PERCENT_MAX &&
           sensors->liquid_percent >= AGRI_SENSOR_PERCENT_MIN &&
           sensors->liquid_percent <= AGRI_SENSOR_PERCENT_MAX &&
           sensors->altitude_m >= AGRI_SENSOR_ALTITUDE_MIN_M &&
           sensors->altitude_m <= AGRI_SENSOR_ALTITUDE_MAX_M;
}

static uint32_t update_faults(uint32_t previous, const AgriSensorData *sensors)
{
    uint32_t faults = previous;

    if (!sensor_data_valid(sensors)) {
        return faults | AGRI_FAULT_SENSOR_RANGE;
    }
    faults &= ~AGRI_FAULT_SENSOR_RANGE;

    if (sensors->battery_percent <= AGRI_BATTERY_TRIP_PERCENT) {
        faults |= AGRI_FAULT_LOW_BATTERY;
    } else if (sensors->battery_percent >= AGRI_BATTERY_RECOVER_PERCENT) {
        faults &= ~AGRI_FAULT_LOW_BATTERY;
    }

    if (sensors->liquid_percent <= AGRI_LIQUID_TRIP_PERCENT) {
        faults |= AGRI_FAULT_LOW_LIQUID;
    } else if (sensors->liquid_percent >= AGRI_LIQUID_RECOVER_PERCENT) {
        faults &= ~AGRI_FAULT_LOW_LIQUID;
    }

    if (sensors->altitude_m < AGRI_ALTITUDE_LOW_TRIP_M) {
        faults |= AGRI_FAULT_ALTITUDE_LOW;
    } else if (sensors->altitude_m >= AGRI_ALTITUDE_LOW_RECOVER_M) {
        faults &= ~AGRI_FAULT_ALTITUDE_LOW;
    }

    if (sensors->altitude_m > AGRI_ALTITUDE_HIGH_TRIP_M) {
        faults |= AGRI_FAULT_ALTITUDE_HIGH;
    } else if (sensors->altitude_m <= AGRI_ALTITUDE_HIGH_RECOVER_M) {
        faults &= ~AGRI_FAULT_ALTITUDE_HIGH;
    }

    return faults;
}

void AgriControl_Init(AgriControl *control)
{
    if (control != 0) {
        control->state = AGRI_STATE_INIT;
        control->active_faults = AGRI_FAULT_NONE;
        control->latched_faults = AGRI_FAULT_NONE;
    }
}

AgriControlOutput AgriControl_Update(AgriControl *control,
                                    const AgriSensorData *sensors,
                                    const AgriControlInput *input)
{
    AgriControlOutput output = {AGRI_STATE_SAFE_STOP, AGRI_FAULT_SENSOR_RANGE,
                                AGRI_FAULT_SENSOR_RANGE, 0u, true};
    uint8_t requested_pwm;

    if (control == 0 || input == 0) {
        return output;
    }

    control->active_faults = update_faults(control->active_faults, sensors);
    if (control->active_faults != AGRI_FAULT_NONE) {
        control->latched_faults |= control->active_faults;
        control->state = AGRI_STATE_SAFE_STOP;
    } else if (control->state == AGRI_STATE_SAFE_STOP) {
        if (input->acknowledge) {
            control->latched_faults = AGRI_FAULT_NONE;
            control->state = AGRI_STATE_READY;
        }
    } else if (control->state == AGRI_STATE_INIT) {
        control->state = AGRI_STATE_READY;
    } else if (input->spray_requested) {
        control->state = AGRI_STATE_SPRAYING;
    } else {
        control->state = AGRI_STATE_READY;
    }

    requested_pwm = input->requested_pwm_percent;
    if (requested_pwm == 0u) {
        requested_pwm = AGRI_DEFAULT_SPRAY_PWM_PERCENT;
    } else if (requested_pwm > 100u) {
        requested_pwm = 100u;
    }

    output.state = control->state;
    output.active_faults = control->active_faults;
    output.latched_faults = control->latched_faults;
    output.alarm_active = control->state == AGRI_STATE_SAFE_STOP;
    output.pump_pwm_percent = control->state == AGRI_STATE_SPRAYING
                                  ? requested_pwm
                                  : 0u;
    return output;
}

const char *AgriControl_StateName(AgriSystemState state)
{
    switch (state) {
    case AGRI_STATE_INIT: return "INIT";
    case AGRI_STATE_READY: return "READY";
    case AGRI_STATE_SPRAYING: return "SPRAYING";
    case AGRI_STATE_WARNING: return "WARNING";
    case AGRI_STATE_SAFE_STOP: return "SAFE_STOP";
    default: return "UNKNOWN";
    }
}
