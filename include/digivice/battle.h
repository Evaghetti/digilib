#ifndef BATTLE_H
#define BATTLE_H

#include "digitype.h"
#include "player.h"

uint8_t DIGIVICE_canBattle(const player_t* pstPlayer);

uint8_t DIGIVICE_tryBattle(player_t* pstPlayer);

uint8_t DIGIVICE_updateBattle(player_t* pstPlayer, uint32_t uideltaTime);

void DIGIVICE_renderBattleBanner();

void DIGIVICE_renderBattle(const player_t* pstPlayer);

#endif
