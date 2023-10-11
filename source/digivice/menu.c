#include "menu.h"

#include "render.h"
#include "sprites.h"

#define X_POSITION_ITEMS              4
#define X_POSITION_SELECTOR_TWO_LINES 1
#define X_POSITION_SELECTOR_SPRITE    3

void DIGIVICE_initMenu(menu_t* pstMenu, uint8_t uiCountItems,
                       const menu_item_t* pstItems) {
    pstMenu->uiCountItems = uiCountItems;
    pstMenu->uiCurrentIndex = 0;

    pstMenu->pstItems = pstItems;
    pstMenu->fInUse = 1;
}

void DIGIVICE_advanceMenu(menu_t* pstMenu, direction_menu_e eDirection) {
    switch (eDirection) {
        case MENU_DIRECTION_FORWARD:
            pstMenu->uiCurrentIndex++;
            if (pstMenu->uiCurrentIndex >= pstMenu->uiCountItems)
                pstMenu->uiCurrentIndex = 0;
            break;
        case MENU_DIRECTION_BACKWARD:
            if (pstMenu->uiCurrentIndex == 0)
                pstMenu->uiCurrentIndex = pstMenu->uiCountItems;
            pstMenu->uiCurrentIndex--;
            break;
    }
}

void DIGIVICE_drawMenuTwoLines(const menu_t* pstMenu) {
    const uint8_t uiStart = pstMenu->uiCurrentIndex & 0xFE;
    uint8_t i;

    for (i = uiStart; i < uiStart + 2 && i < pstMenu->uiCountItems; i++) {
        const menu_item_t* const pstItem = &pstMenu->pstItems[i];
        uint8_t uiPosition = (i & 1) << 3;

        if (i == pstMenu->uiCurrentIndex)
            DIGIVICE_drawTile(SELECTOR_TILE, X_POSITION_SELECTOR_TWO_LINES,
                              uiPosition, EFFECT_NONE);

        switch (pstItem->eType) {
            case MENU_ITEM_TEXT:
                DIGIVICE_drawText((char*)pstItem->pDataItem, X_POSITION_ITEMS,
                                  uiPosition, EFFECT_NONE);
                break;
            case MENU_ITEM_IMAGE:
                DIGIVICE_drawSprite((uint16_t* const)pstItem->pDataItem, 8, 0,
                                    EFFECT_NONE);
                break;
        }
    }
}

void DIGIVICE_drawMenuSprite(const menu_t* pstMenu) {
    const menu_item_t* const pstItem =
        &pstMenu->pstItems[pstMenu->uiCurrentIndex];
    const uint16_t* const puiSprite = pstItem->pDataItem;

    DIGIVICE_drawTile(SELECTOR_TILE, X_POSITION_SELECTOR_SPRITE, 4,
                      EFFECT_NONE);
    DIGIVICE_drawSprite(puiSprite, 8, 0, EFFECT_NONE);
}
