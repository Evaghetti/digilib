#ifndef CLOCK_H
#define CLOCK_H

#include "digitype.h"

typedef enum pass_time_selector_t { MINUTES, HOUR } pass_time_selector_t;

uint16_t DIGIVICE_getTime();

void DIGIVICE_setTime(uint16_t uiComputedTime);

uint8_t DIGIVICE_minutesPassed();

void DIGIVICE_toggleSetTime();

void DIGIVICE_passTime(pass_time_selector_t ePassTime);

void DIGIVICE_updateClock(uint16_t uiDeltaTime);

void DIGIVICE_renderClock();

#endif  // CLOCK_H
