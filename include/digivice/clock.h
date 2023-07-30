#ifndef CLOCK_H
#define CLOCK_H

#include "digitype.h"

uint8_t DIGIVICE_minutesPassed();

void DIGIVICE_updateClock(uint16_t uiDeltaTime, uint8_t fIsConfiguring);

void DIGIVICE_renderClock();

#endif  // CLOCK_H
