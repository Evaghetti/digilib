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

typedef enum battle_animation_state_e {
    WAITING,
    SHOOTING,
    HIT,
    POST_ANIMATION,
    POST_ANIMATION_WASH
} battle_animation_state_e;

static uint16_t uiPassedTime = 0;
static uint8_t uiProjectilePos = 8, uiCurrentProjectile = 0, uiOffset = 0;
static int8_t iProjectileDirection = -1;
static battle_animation_state_e eCurrentState = WAITING;

uint8_t DIGIVICE_canBattle(const player_t* pstPlayer) {
    return DIGIBATTLE_canBattle(pstPlayer->pstPet);
}

uint8_t DIGIVICE_tryBattle(player_t* pstPlayer) {
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
    return uiRet;
}

inline static void updateMovimentProjectile(const player_t* pstPlayer) {
    if (uiPassedTime >= STEP_PROJECTILE) {
        uiProjectilePos += iProjectileDirection;

        if (uiProjectilePos == 0xF8)  // -8
            iProjectileDirection = 1;
        else if (uiProjectilePos == 8) {
            iProjectileDirection = -1;
            uiProjectilePos = 0;
            uiCurrentProjectile++;

            if (uiCurrentProjectile < 4 || pstPlayer->eState == LOSE_BATTLE)
                eCurrentState = HIT;
            else
                eCurrentState = WAITING;
        }

        uiPassedTime = 0;
    }
}

inline static void updateHitAnimation() {
    if (uiPassedTime >= STEP_HIT) {
        uiProjectilePos++;
        if (uiProjectilePos == 5) {
            uiProjectilePos = 0;
            eCurrentState = uiCurrentProjectile >= 4 ? POST_ANIMATION : WAITING;
        }

        uiPassedTime = 0;
    }
}

inline static void updateWaiting() {
    if (uiPassedTime >= STEP_WAITING) {
        uiProjectilePos = 8;

        if (uiCurrentProjectile >= 4) {
            eCurrentState = POST_ANIMATION;
            uiProjectilePos = 0;
        } else {
            eCurrentState = SHOOTING;
        }

        uiPassedTime = 0;
    }
}

inline static uint8_t updatePostAnimation(const player_t* pstPlayer) {
    uint8_t uiFinished = 0;
    if (uiPassedTime >= STEP_BLINK) {
        uiProjectilePos++;

        const uint8_t uiStopCondition =
            pstPlayer->eState == LOSE_BATTLE ? 5 : 6;
        if (uiProjectilePos == uiStopCondition) {
            if (pstPlayer->eState == LOSE_BATTLE)
                eCurrentState = POST_ANIMATION_WASH;
            else
                uiFinished = 1;
        }

        uiPassedTime = 0;
    }

    return uiFinished;
}

inline static uint8_t updatePostAnimationWash() {
    uint8_t uiFinished = 0;
    if (uiPassedTime >= STEP_WASH) {
        uiOffset += 1;

        if (uiOffset >= 40)
            uiFinished = 1;

        uiPassedTime = 0;
    }

    return uiFinished;
}

uint8_t DIGIVICE_updateBattle(player_t* pstPlayer, uint32_t uideltaTime) {
    uint8_t uiFinished = 0;

    uiPassedTime += uideltaTime;

    switch (eCurrentState) {
        case WAITING:
            updateWaiting();
            break;
        case SHOOTING:
            updateMovimentProjectile(pstPlayer);
            break;
        case HIT:
            updateHitAnimation();
            break;
        case POST_ANIMATION:
            uiFinished = updatePostAnimation(pstPlayer);
            break;
        case POST_ANIMATION_WASH:
            uiFinished = updatePostAnimationWash();
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

inline static void renderShooting(const player_t* pstPlayer) {
    const uint8_t uiFlippedProjectile = iProjectileDirection < 0;
    const uint8_t uiIndexDigimon = uiFlippedProjectile ? 1 : 0;

    DIGIVICE_drawTile(
        guiDigimonProjectileSprites[pstPlayer->pstPet->uiIndexCurrentDigimon -
                                    1],
        uiProjectilePos, 0, !uiFlippedProjectile);
    if (uiCurrentProjectile == 3 &&
        ((uiFlippedProjectile && pstPlayer->eState == WIN_BATTLE) ||
         (!uiFlippedProjectile && pstPlayer->eState == LOSE_BATTLE))) {

        DIGIVICE_drawTile(guiDigimonProjectileSprites
                              [pstPlayer->pstPet->uiIndexCurrentDigimon - 1],
                          uiProjectilePos, 8, !uiFlippedProjectile);
    }

    DIGIVICE_drawSprite(
        guiDigimonAnimationDatabase[pstPlayer->pstPet->uiIndexCurrentDigimon -
                                    1][2][uiIndexDigimon],
        16, 0, EFFECT_NONE);
}

inline static void renderPostAnimation(const player_t* pstPlayer,
                                       uint8_t uiHappy) {
    uint8_t uiShowEffect = uiProjectilePos & 1;
    const uint8_t* uiEffecTile =
        uiHappy ? HAPPY_SUN_TILE : guiSkullAnimation[0];
    const uint8_t uiAnimation = uiHappy ? 1 : 5;

    if (uiShowEffect) {
        DIGIVICE_drawTile(uiEffecTile, -uiOffset, 0, EFFECT_NONE);
        DIGIVICE_drawTile(uiEffecTile, 24 - uiOffset, 0, EFFECT_NONE);
        DIGIVICE_drawTile(uiEffecTile, -uiOffset, 8, EFFECT_NONE);
        DIGIVICE_drawTile(uiEffecTile, 24 - uiOffset, 8, EFFECT_NONE);
    }

    if (!uiHappy) {
        DIGIVICE_drawTile(CLEANING_TILE, 32 - uiOffset, 0, EFFECT_NONE);
        DIGIVICE_drawTile(CLEANING_TILE, 32 - uiOffset, 8, EFFECT_NONE);
    }

    DIGIVICE_drawSprite(
        guiDigimonAnimationDatabase[pstPlayer->pstPet->uiIndexCurrentDigimon -
                                    1][uiAnimation][uiProjectilePos >= 1],
        8 - uiOffset, 0, EFFECT_NONE);
}

void DIGIVICE_renderBattle(const player_t* pstPlayer) {
    switch (eCurrentState) {
        case WAITING:
            DIGIVICE_drawSprite(
                guiDigimonAnimationDatabase
                    [pstPlayer->pstPet->uiIndexCurrentDigimon - 1][2][0],
                16, 0, uiCurrentProjectile >= 4);
            break;
        case SHOOTING:
            renderShooting(pstPlayer);
            break;
        case HIT:
            DIGIVICE_drawPopup(guiDamagePopuoAnimation[uiProjectilePos & 1]);
            break;
        case POST_ANIMATION:
        case POST_ANIMATION_WASH:
            renderPostAnimation(pstPlayer, pstPlayer->eState == WIN_BATTLE);
            break;
        default:
            break;
    }
}
