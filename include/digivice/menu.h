#ifndef MENU_H
#define MENU_H

#include "SDL2/SDL_image.h"

typedef struct Option {
    char text[50];
    SDL_Texture* texture;
    SDL_Rect spriteRect;
} Option;

typedef enum TypeMenu { UNDEFINED, TEXT, IMAGE } TypeMenu;

typedef struct Menu {
    int countOptions, currentOption;
    TypeMenu type;
    Option* options;
} Menu;

Menu initMenuText(int count, char* texts[]);

Menu initMenuImage(int count, char* paths[], SDL_Rect spriteRects[]);

Menu initMenuImageRaw(int count, SDL_Texture* textures[]);

void advanceMenu(Menu* menu, int step);

void drawMenu(SDL_Renderer* renderer, Menu* menu);

void freeMenu(Menu* menu);

#endif