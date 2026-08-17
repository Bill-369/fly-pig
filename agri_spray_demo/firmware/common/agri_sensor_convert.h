#ifndef AGRI_SENSOR_CONVERT_H
#define AGRI_SENSOR_CONVERT_H

#include <stdint.h>
#include "agri_types.h"

/* SIMULATED SENSOR INPUT conversions for 0..3.3 V, 12-bit ADC channels. */
float AgriSensor_AdcToPercent(uint16_t adc_counts);
float AgriSensor_AdcToAltitudeM(uint16_t adc_counts);
AgriSensorData AgriSensor_ConvertSimulated(uint16_t battery_adc,
                                           uint16_t liquid_adc,
                                           uint16_t altitude_adc);

#endif
