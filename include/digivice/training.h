#ifndef TRAINING_H
#define TRAINING_H

#include "player.h"

void DIGIVICE_initTraining(player_t* pstPlayer);

void DIGIVICE_updateTraining(uint16_t uiDeltaTime);

uint8_t DIGIVICE_handleInputTraining();

void DIGIVICE_renderTraining();

#endif  // TRAINING_H