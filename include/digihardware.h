#ifndef DIGIHARDWARE_H
#define DIGIHARDWARE_H

#include <stdint.h>
#include "digimon.h"

uint16_t DIGIHW_setTime();

uint16_t DIGIHW_timeMinutes();

void DIGIHW_addTime(uint16_t uiAddingAmount);

uint8_t DIGIHW_randomNumber();

uint8_t DIGIHW_readDigimon(const char* szFileName,
                           playing_digimon_t* pstPlayingDigimon);

uint8_t DIGIHW_saveDigimon(const char* szFileName,
                           playing_digimon_t* pstPlayingDigimon);

#endif