#ifndef BATTLE_H
#define BATTLE_H

#include "player.h"

uint8_t DIGIVICE_canBattle(const player_t* pstPlayer);

uint8_t DIGIVICE_tryBattle(player_t* pstPlayer);

void DIGIVICE_renderBattleBanner();

#endif