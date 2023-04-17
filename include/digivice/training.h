#ifndef TRAINING_H
#define TRAINING_H

#include "digimon.h"

void DIGIVICE_initTraining(playing_digimon_t* pstPlayingDigimon);

void DIGIVICE_updateTraining(uint16_t uiDeltaTime);

void DIGIVICE_handleInputTraining();

void DIGIVICE_renderTraining();

#endif  // TRAINING_H