#include "battle.h"

#include "digibattle_classic.h"
#include "digivice.h"
#include "digivice_hal.h"
#include "enums_digivice.h"
#include "player.h"
#include "render.h"
#include "sprites.h"

#define STEP_PROJECTILE 50
#define STEP_HIT        150
#define STEP_WAITING    (STEP_HIT * 5)
#define STEP_BLINK      500
#define STEP_WASH       20

void DIGIVICE_initBattle() {}

uint8_t DIGIVICE_canBattle(const player_t* pstPlayer) {
    return DIGIBATTLE_canBattle(pstPlayer->pstPet);
}

uint8_t DIGIVICE_tryBattle(player_t* pstPlayer,
                           battle_animation_t* pstBattleAnimation) {
    uint8_t uiRet = DIGIVICE_canBattle(pstPlayer);

    if (uiRet != DIGIBATTLE_RET_OK) {
        LOG("No longer can battle -> %d", uiRet);
        return DIGIVICE_CANCEL_BATTLE;
    }

    LOG("Going to start battle routines -> %d",
        DIGIVICE_isButtonPressed(BUTTON_B));
    uiRet = (DIGIVICE_isButtonPressed(BUTTON_B)
                 ? DIGIBATTLE_initiate(pstPlayer->pstPet)
                 : DIGIBATTLE_continue(pstPlayer->pstPet));
    switch (uiRet) {
        case DIGIBATTLE_RET_POLL:
            LOG("No data polled");
            return DIGIVICE_POLL_BATTLE;
        case DIGIBATTLE_RET_ERROR:
            LOG("Error during battle -> %d", uiRet);
            return uiRet;
    }

    LOG("Finished battle, result -> %d", uiRet);

    DIGIBATTLE_changeStats(pstPlayer->pstPet, uiRet);
    pstPlayer->eState = uiRet == DIGIBATTLE_RET_WIN ? WIN_BATTLE : LOSE_BATTLE;

    battle_animation_t data = {.eCurrentState = WAITING,
                               .uiPassedTime = 0,
                               .uiOffset = 0,
                               .iProjectileDirection = -1,
                               .uiProjectilePos = 4,
                               .uiCurrentProjectile = 0};
    *pstBattleAnimation = data;

    return uiRet;
}

inline static void updateMovimentProjectile(
    battle_animation_t* pstBattleAnimation, const player_t* pstPlayer) {
    if (pstBattleAnimation->uiPassedTime >= STEP_PROJECTILE) {
        pstBattleAnimation->uiProjectilePos +=
            pstBattleAnimation->iProjectileDirection;

        if (pstBattleAnimation->uiProjectilePos == 0xF8)  // -8
            pstBattleAnimation->iProjectileDirection = 1;
        else if (pstBattleAnimation->uiProjectilePos == 8) {
            pstBattleAnimation->iProjectileDirection = -1;
            pstBattleAnimation->uiProjectilePos = 0;
            pstBattleAnimation->uiCurrentProjectile++;

            if (pstBattleAnimation->uiCurrentProjectile < 4 ||
                pstPlayer->eState == LOSE_BATTLE)
                pstBattleAnimation->eCurrentState = HIT;
            else
                pstBattleAnimation->eCurrentState = WAITING;
        }

        pstBattleAnimation->uiPassedTime = 0;
    }
}

inline static void updateHitAnimation(battle_animation_t* pstBattleAnimation) {
    if (pstBattleAnimation->uiPassedTime >= STEP_HIT) {
        pstBattleAnimation->uiProjectilePos++;
        if (pstBattleAnimation->uiProjectilePos == 5) {
            pstBattleAnimation->uiProjectilePos = 0;
            pstBattleAnimation->eCurrentState =
                pstBattleAnimation->uiCurrentProjectile >= 4 ? POST_ANIMATION
                                                             : WAITING;
        }

        pstBattleAnimation->uiPassedTime = 0;
    }
}

inline static void updateWaiting(battle_animation_t* pstBattleAnimation) {
    if (pstBattleAnimation->uiPassedTime >= STEP_WAITING) {
        pstBattleAnimation->uiProjectilePos = 8;

        if (pstBattleAnimation->uiCurrentProjectile >= 4) {
            pstBattleAnimation->eCurrentState = POST_ANIMATION;
            pstBattleAnimation->uiProjectilePos = 0;
        } else {
            pstBattleAnimation->eCurrentState = SHOOTING;
        }

        pstBattleAnimation->uiPassedTime = 0;
    }
}

inline static uint8_t updatePostAnimation(
    battle_animation_t* pstBattleAnimation, const player_t* pstPlayer) {
    uint8_t uiFinished = 0;
    if (pstBattleAnimation->uiPassedTime >= STEP_BLINK) {
        pstBattleAnimation->uiProjectilePos++;

        const uint8_t uiStopCondition =
            pstPlayer->eState == LOSE_BATTLE ? 5 : 6;
        if (pstBattleAnimation->uiProjectilePos == uiStopCondition) {
            if (pstPlayer->eState == LOSE_BATTLE)
                pstBattleAnimation->eCurrentState = POST_ANIMATION_WASH;
            else
                uiFinished = 1;
        }

        pstBattleAnimation->uiPassedTime = 0;
    }

    return uiFinished;
}

inline static uint8_t updatePostAnimationWash(
    battle_animation_t* pstBattleAnimation) {
    uint8_t uiFinished = 0;
    if (pstBattleAnimation->uiPassedTime >= STEP_WASH) {
        pstBattleAnimation->uiOffset += 1;

        if (pstBattleAnimation->uiOffset >= 40)
            uiFinished = 1;

        pstBattleAnimation->uiPassedTime = 0;
    }

    return uiFinished;
}

uint8_t DIGIVICE_updateBattle(battle_animation_t* pstBattleAnimation,
                              player_t* pstPlayer, uint32_t uideltaTime) {
    uint8_t uiFinished = 0;

    pstBattleAnimation->uiPassedTime += uideltaTime;

    switch (pstBattleAnimation->eCurrentState) {
        case WAITING:
            updateWaiting(pstBattleAnimation);
            break;
        case SHOOTING:
            updateMovimentProjectile(pstBattleAnimation, pstPlayer);
            break;
        case HIT:
            updateHitAnimation(pstBattleAnimation);
            break;
        case POST_ANIMATION:
            uiFinished = updatePostAnimation(pstBattleAnimation, pstPlayer);
            break;
        case POST_ANIMATION_WASH:
            uiFinished = updatePostAnimationWash(pstBattleAnimation);
            break;
        default:
            break;
    }

    if (uiFinished) {
        DIGIVICE_changeStatePlayer(pstPlayer, WALKING);
        LOG("Finished");
    }

    return uiFinished;
}

void DIGIVICE_renderBattleBanner() {
    DIGIVICE_drawPopup(guiBattlePopupSprite);
}

inline static void renderShooting(const battle_animation_t* pstBattleAnimation,
                                  const player_t* pstPlayer) {
    const uint8_t uiFlippedProjectile =
        pstBattleAnimation->iProjectileDirection < 0;
    const uint8_t uiIndexDigimon = uiFlippedProjectile ? 1 : 0;

    DIGIVICE_drawTile(
        guiDigimonProjectileSprites[pstPlayer->pstPet->uiIndexCurrentDigimon -
                                    1],
        pstBattleAnimation->uiProjectilePos, 0, !uiFlippedProjectile);
    if (pstBattleAnimation->uiCurrentProjectile == 3 &&
        ((uiFlippedProjectile && pstPlayer->eState == WIN_BATTLE) ||
         (!uiFlippedProjectile && pstPlayer->eState == LOSE_BATTLE))) {

        DIGIVICE_drawTile(guiDigimonProjectileSprites
                              [pstPlayer->pstPet->uiIndexCurrentDigimon - 1],
                          pstBattleAnimation->uiProjectilePos, 8,
                          !uiFlippedProjectile);
    }

    DIGIVICE_drawSprite(
        guiDigimonAnimationDatabase[pstPlayer->pstPet->uiIndexCurrentDigimon -
                                    1][2][uiIndexDigimon],
        16, 0, EFFECT_NONE);
}

inline static void renderPostAnimation(
    const battle_animation_t* pstBattleAnimation, const player_t* pstPlayer,
    uint8_t uiHappy) {
    uint8_t uiShowEffect = pstBattleAnimation->uiProjectilePos & 1;
    const uint8_t* uiEffecTile =
        uiHappy ? HAPPY_SUN_TILE : guiSkullAnimation[0];
    const uint8_t uiAnimation = uiHappy ? 1 : 5;

    if (uiShowEffect) {
        DIGIVICE_drawTile(uiEffecTile, -pstBattleAnimation->uiOffset, 0,
                          EFFECT_NONE);
        DIGIVICE_drawTile(uiEffecTile, 24 - pstBattleAnimation->uiOffset, 0,
                          EFFECT_NONE);
        DIGIVICE_drawTile(uiEffecTile, -pstBattleAnimation->uiOffset, 8,
                          EFFECT_NONE);
        DIGIVICE_drawTile(uiEffecTile, 24 - pstBattleAnimation->uiOffset, 8,
                          EFFECT_NONE);
    }

    if (!uiHappy) {
        DIGIVICE_drawTile(CLEANING_TILE, 32 - pstBattleAnimation->uiOffset, 0,
                          EFFECT_NONE);
        DIGIVICE_drawTile(CLEANING_TILE, 32 - pstBattleAnimation->uiOffset, 8,
                          EFFECT_NONE);
    }

    DIGIVICE_drawSprite(
        guiDigimonAnimationDatabase[pstPlayer->pstPet->uiIndexCurrentDigimon -
                                    1][uiAnimation]
                                   [pstBattleAnimation->uiProjectilePos >= 1],
        8 - pstBattleAnimation->uiOffset, 0, EFFECT_NONE);
}

void DIGIVICE_renderBattle(const battle_animation_t* pstBattleAnimation,
                           const player_t* pstPlayer) {
    switch (pstBattleAnimation->eCurrentState) {
        case WAITING:
            DIGIVICE_drawSprite(
                guiDigimonAnimationDatabase
                    [pstPlayer->pstPet->uiIndexCurrentDigimon - 1][2][0],
                16, 0, pstBattleAnimation->uiCurrentProjectile >= 4);
            break;
        case SHOOTING:
            renderShooting(pstBattleAnimation, pstPlayer);
            break;
        case HIT:
            DIGIVICE_drawPopup(
                guiDamagePopuoAnimation[pstBattleAnimation->uiProjectilePos &
                                        1]);
            break;
        case POST_ANIMATION:
        case POST_ANIMATION_WASH:
            renderPostAnimation(pstBattleAnimation, pstPlayer,
                                pstPlayer->eState == WIN_BATTLE);
            break;
        default:
            break;
    }
}
