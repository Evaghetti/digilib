#include "player.h"

#include "enums.h"

#include "render.h"
#include "sprites.h"

#define SIZE_OF_ARRAY(x) (sizeof(x) / sizeof(x[0]))

static const int8_t uiWalkCycleMove[] = {-2, -2, 0, 2, 0, -2, -2, 2, -2, -2, 2, 0, 0, 0, 2, 2, 2, 2, 2, 0, -2, 0, 2, 0, 2, 2, -2, 2, -2, -2, -2, -2};
static const uint8_t uiWalkCycleFlips[] = {0, 0, 1, 1, 0, 0, 0, 1, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 0, 0, 1, 1, 0, 1, 1, 1, 1, 1, 0, 0, 0};
static const uint8_t uiWalkCycleIndices[] = {0, 1, 1, 0, 0, 1, 1, 1, 1, 2, 1, 2, 1, 1, 1, 0, 0, 0, 1, 1, 0, 0, 0, 1, 1, 2, 1, 2, 1, 1, 0, 0};

int DIGIVICE_initPlayer(player_t* pstPlayer) {
    pstPlayer->uiPosition = (LCD_SCREEN_WIDTH / 2 - TILE_WIDTH);
    pstPlayer->uiCurrentFrame = 0xFF; // Overflow on first frame and goes back to zero.
    return DIGI_RET_OK;
}

int DIGIVICE_updatePlayer(player_t* pstPlayer, uint32_t uiDeltaTime) {
    pstPlayer->uiDeltaTime += uiDeltaTime;

    if (pstPlayer->uiDeltaTime >= 500) {
        pstPlayer->uiDeltaTime = 0;

        pstPlayer->uiCurrentFrame++;
        if (pstPlayer->uiCurrentFrame >= SIZE_OF_ARRAY(uiWalkCycleMove))
            pstPlayer->uiCurrentFrame = 0;
        pstPlayer->uiFlipped = uiWalkCycleFlips[pstPlayer->uiCurrentFrame];
        pstPlayer->uiPosition += uiWalkCycleMove[pstPlayer->uiCurrentFrame];
    }
    return DIGI_RET_OK;
}

void DIGIVICE_renderPlayer(const player_t* pstPlayer) {
    const uint16_t* const puiSprite = guiDigimonWalkingAnimationDatabase
        [pstPlayer->pstPet->uiIndexCurrentDigimon]
        [uiWalkCycleIndices[pstPlayer->uiCurrentFrame]];

    DIGIVICE_drawSprite(puiSprite, pstPlayer->uiPosition, 0,
                        pstPlayer->uiFlipped);
}
