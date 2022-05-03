#ifndef API_H
#define API_H

#include "digimon.h"

int DIGI_init(const char* szSaveFile);

uint8_t DIGI_updateEventsDeltaTime(uint16_t uiDeltaTime, uint8_t* puiEvents);

void DIGI_cleanWaste();

#endif  // API_H