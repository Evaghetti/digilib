#ifndef MENU_H
#define MENU_H

#include "SDL2/SDL_image.h"

typedef struct Option {
    char text[50];
    SDL_Texture* texture;
    SDL_Rect spriteRect;
} Option;

typedef enum TypeMenu { UNDEFINED, TEXT, IMAGE } TypeMenu;

typedef enum CustomizationMenu {
    NONE,
    NO_CURSOR = 1 << 0,
    FILL_SCREEN = 1 << 1,
} CustomizationMenu;

typedef struct Menu {
    int countOptions, currentOption;
    CustomizationMenu customs;
    TypeMenu type;
    Option* options;
    SDL_Texture* header;
} Menu;

Menu initMenu(int count, TypeMenu type);

Menu initMenuText(int count, char* texts[]);

Menu initMenuImage(int count, char* paths[], SDL_Rect spriteRects[]);

Menu initMenuImageRaw(int count, SDL_Texture* textures[]);

void addMenuImage(Menu* menu, const char* path, SDL_Rect spriteRect);

void advanceMenu(Menu* menu, int step);

void drawMenu(SDL_Renderer* renderer, Menu* menu);

void freeMenu(Menu* menu);

#endif