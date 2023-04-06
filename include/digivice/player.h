#ifndef PLAYER_H
#define PLAYER_H

#include "digimon.h"

typedef enum player_state_e {
    WALKING,
    WAITING_HATCH,
    HATCHING,
    EVOLVING,
    EATING,
    EATING_VITAMIN,
    NEED_SLEEP,
} player_state_e;

typedef struct player_t {
    playing_digimon_t* pstPet;
    uint8_t uiPosition, uiFlipped;
    uint8_t uiCurrentFrame;
    uint16_t uiDeltaTimeLib, uiDeltaTimeStep, uiCurrentStep;
    uint16_t uiIndexBeforeEvolve;
    player_state_e eState;
} player_t;

int DIGIVICE_initPlayer(player_t* pstPlayer);

int DIGIVICE_updatePlayer(player_t* pstPlayer, uint32_t uiDeltaTime);

void DIGIVICE_renderPlayer(const player_t* pstPlayer);

uint8_t DIGIVICE_changeStatePlayer(player_t* pstPlayer,
                                   player_state_e eNewState);

#endif