#ifndef INFO_H
#define INFO_H

#include "digimon.h"

void DIGIVICE_initInfoDisplay(const playing_digimon_t* pstPlayerData);

void DIGIVICE_updateInfoDisplay(uint32_t uiDeltaTime);

void DIGIVICE_renderInfoDisplay();

#endif