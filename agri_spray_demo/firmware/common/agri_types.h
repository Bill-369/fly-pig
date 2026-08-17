#ifndef AGRI_TYPES_H
#define AGRI_TYPES_H

#include <stdbool.h>
#include <stdint.h>

typedef struct {
    float battery_percent;
    float liquid_percent;
    float altitude_m;
} AgriSensorData;

typedef enum {
    AGRI_STATE_INIT = 0,
    AGRI_STATE_READY,
    AGRI_STATE_SPRAYING,
    AGRI_STATE_WARNING,
    AGRI_STATE_SAFE_STOP
} AgriSystemState;

typedef enum {
    AGRI_FAULT_NONE          = 0u,
    AGRI_FAULT_LOW_BATTERY   = 1u << 0,
    AGRI_FAULT_LOW_LIQUID    = 1u << 1,
    AGRI_FAULT_ALTITUDE_LOW  = 1u << 2,
    AGRI_FAULT_ALTITUDE_HIGH = 1u << 3,
    AGRI_FAULT_SENSOR_RANGE  = 1u << 4
} AgriFault;

typedef struct {
    bool spray_requested;
    bool acknowledge;
    uint8_t requested_pwm_percent;
} AgriControlInput;

typedef struct {
    AgriSystemState state;
    uint32_t active_faults;
    uint32_t latched_faults;
    uint8_t pump_pwm_percent;
    bool alarm_active;
} AgriControlOutput;

typedef struct {
    AgriSystemState state;
    uint32_t active_faults;
    uint32_t latched_faults;
} AgriControl;

#endif
