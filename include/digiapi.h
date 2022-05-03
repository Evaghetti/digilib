#ifndef API_H
#define API_H

#include "digimon.h"

uint8_t DIGI_updateEventsDeltaTime(uint16_t uiDeltaTime, uint8_t* puiEvents,
                                   playing_digimon_t* pstPlayingDigimon);

void DIGI_cleanWaste();

#endif  // API_H