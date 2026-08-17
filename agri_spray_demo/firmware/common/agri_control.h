#ifndef AGRI_CONTROL_H
#define AGRI_CONTROL_H

#include "agri_types.h"

void AgriControl_Init(AgriControl *control);
AgriControlOutput AgriControl_Update(AgriControl *control,
                                    const AgriSensorData *sensors,
                                    const AgriControlInput *input);
const char *AgriControl_StateName(AgriSystemState state);

#endif
