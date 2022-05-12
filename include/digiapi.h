#ifndef API_H
#define API_H

#include "digimon.h"

int DIGI_init(const char* szSaveFile);

uint8_t DIGI_updateEventsDeltaTime(uint16_t uiDeltaTime, uint8_t* puiEvents);

void DIGI_cleanWaste();

playing_digimon_t DIGI_playingDigimon();

digimon_t** DIGI_possibleDigitama(uint8_t* puiCount);

void DIGI_saveGame();

#endif  // API_H