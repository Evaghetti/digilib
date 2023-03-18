#ifndef PLAYER_H
#define PLAYER_H

#include "digimon.h"

typedef struct player_t {
    playing_digimon_t* pstPet;
    uint8_t uiPosition, uiFlipped;
    uint8_t uiState, uiCurrentFrame;
    uint32_t uiDeltaTime;
} player_t;

int DIGIVICE_initPlayer(player_t* pstPlayer);

int DIGIVICE_updatePlayer(player_t* pstPlayer, uint32_t uiDeltaTime);

void DIGIVICE_renderPlayer(const player_t* pstPlayer);

#endif