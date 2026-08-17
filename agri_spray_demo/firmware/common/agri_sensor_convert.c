#include "agri_sensor_convert.h"
#include "agri_config.h"

static uint16_t clamp_adc(uint16_t value)
{
    return value > AGRI_SIM_ADC_MAX_COUNTS ? AGRI_SIM_ADC_MAX_COUNTS : value;
}

float AgriSensor_AdcToPercent(uint16_t adc_counts)
{
    return ((float)clamp_adc(adc_counts) * 100.0f) /
           (float)AGRI_SIM_ADC_MAX_COUNTS;
}

float AgriSensor_AdcToAltitudeM(uint16_t adc_counts)
{
    return ((float)clamp_adc(adc_counts) * AGRI_SIM_ALTITUDE_FULL_SCALE_M) /
           (float)AGRI_SIM_ADC_MAX_COUNTS;
}

AgriSensorData AgriSensor_ConvertSimulated(uint16_t battery_adc,
                                           uint16_t liquid_adc,
                                           uint16_t altitude_adc)
{
    AgriSensorData data;
    data.battery_percent = AgriSensor_AdcToPercent(battery_adc);
    data.liquid_percent = AgriSensor_AdcToPercent(liquid_adc);
    data.altitude_m = AgriSensor_AdcToAltitudeM(altitude_adc);
    return data;
}
