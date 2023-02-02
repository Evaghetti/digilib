#ifndef API_H
#define API_H

#include "digihal.h"
#include "digimon.h"

int DIGI_init(const digihal_t* pstConfig, playing_digimon_t** pstPlayingDigimon);

uint8_t DIGI_initDigimon(playing_digimon_t* pstPlayingDigimon);

uint8_t DIGI_selectDigitama(playing_digimon_t* pstPlayingDigimon,
                            uint8_t uiDigitamaIndex);

uint8_t DIGI_updateEventsDeltaTime(playing_digimon_t* pstPlayingDigimon,
                                   uint16_t uiDeltaTime, uint8_t* puiEvents);

digimon_t** DIGI_possibleDigitama(uint8_t* puiCount);

uint8_t DIGI_readGame(playing_digimon_t* pstPlayingDigimon);

void DIGI_saveGame(const playing_digimon_t* pstPlayingDigimon);

#endif  // API_H