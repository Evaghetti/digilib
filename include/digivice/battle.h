#ifndef BATTLE_H
#define BATTLE_H

#include "digitype.h"
#include "player.h"

typedef enum battle_animation_state_e {
    WAITING,
    SHOOTING,
    HIT,
    POST_ANIMATION,
    POST_ANIMATION_WASH
} battle_animation_state_e;

typedef struct battle_animation_t {
    uint16_t uiPassedTime;
    uint8_t uiProjectilePos, uiCurrentProjectile, uiOffset;
    int8_t iProjectileDirection;
    battle_animation_state_e eCurrentState;
    uint8_t fShownError;
} battle_animation_t;

void DIGIVICE_initBattle();

uint8_t DIGIVICE_canBattle(const player_t* pstPlayer);

uint8_t DIGIVICE_tryBattle(player_t* pstPlayer,
                           battle_animation_t* pstBattleAnimation);

uint8_t DIGIVICE_updateBattle(battle_animation_t* pstBattleAnimation,
                              player_t* pstPlayer, uint32_t uideltaTime);

void DIGIVICE_renderBattleBanner(const battle_animation_t* pstBattleAnimation);

void DIGIVICE_renderBattle(const battle_animation_t* pstBattleAnimation,
                           const player_t* pstPlayer);

#endif
