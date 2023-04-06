#ifndef MENU_H
#define MENU_H

#include "digitype.h"

typedef enum item_type_e { MENU_ITEM_TEXT, MENU_ITEM_IMAGE } item_type_e;

typedef enum direction_menu_e {
    MENU_DIRECTION_BACKWARD,
    MENU_DIRECTION_FORWARD
} direction_menu_e;

typedef struct menu_item_t {
    item_type_e eType;
    void* pDataItem;  // Pointer to tile or string
} menu_item_t;

typedef struct menu_t {
    const menu_item_t* pstItems;
    uint8_t uiCurrentIndex;
    uint8_t uiCountItems;
    uint8_t fInUse;
} menu_t;

void DIGIVICE_initMenu(menu_t* pstMenu, uint8_t uiCountItem,
                       const menu_item_t* pstItems);
void DIGIVICE_advanceMenu(menu_t* pstMenu, direction_menu_e eDirection);
void DIGIVICE_drawMenu(const menu_t* pstMenu);
uint8_t DIGIVICE_isMenuInUse(const menu_t* pstMenu);
#endif