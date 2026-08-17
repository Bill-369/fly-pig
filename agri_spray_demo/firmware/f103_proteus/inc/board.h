#ifndef AGRI_F103_BOARD_H
#define AGRI_F103_BOARD_H

#include <stdbool.h>
#include <stdint.h>

void Board_Init(void);
void Board_ReadAdc(uint16_t *battery, uint16_t *liquid, uint16_t *altitude);
void Board_SetPumpPwm(uint8_t percent);
void Board_SetAlarm(bool active);
bool Board_AckPressed(void);
void Board_LcdShow(const char *line1, const char *line2);
void Board_TelemetryWrite(const char *text);
void Board_DelayMs(uint32_t milliseconds);

#endif
