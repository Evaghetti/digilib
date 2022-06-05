#include "digivice/menu.h"

#include "digivice/globals.h"
#include "digivice/texture.h"

#include <stdarg.h>

static SDL_Texture* cursorTexture = NULL;
static const SDL_Rect cursorClip = {.x = 32, .y = 0, .w = 8, .h = 8};

static SDL_Texture* createTextTexture(SDL_Color color, const char* fmt, ...) {
    char formattedText[50];
    va_list vl;
    va_start(vl, fmt);
    vsnprintf(formattedText, sizeof(formattedText), fmt, vl);
    va_end(vl);

    SDL_Surface* surface = TTF_RenderText_Solid(gFonte, formattedText, color);
    if (surface == NULL) {
        SDL_Log("Error creating surface for message %s", fmt);
        return NULL;
    }

    SDL_Texture* result = SDL_CreateTextureFromSurface(gRenderer, surface);
    if (result == NULL) {
        SDL_Log("Error creating texture for message %s", fmt);
        SDL_FreeSurface(surface);
        return NULL;
    }

    SDL_FreeSurface(surface);
    return result;
}

Menu initMenuText(int count, char* texts[]) {
    Menu ret = {.countOptions = count, .type = TEXT, .currentOption = 0};
    int i;

    ret.options = calloc(count, sizeof(Option));

    for (i = 0; i < count; i++) {
        const char* currentArg = texts[i];

        strncpy(ret.options[i].text, currentArg, sizeof(ret.options[i].text));
    }

    if (cursorTexture == NULL)
        cursorTexture = loadTexture("resource/hud.gif");

    return ret;
}

Menu initMenuImage(int count, char* paths[], SDL_Rect spriteRects[]) {
    Menu ret = {.countOptions = count, .type = IMAGE, .currentOption = 0};
    int i;

    ret.options = calloc(count, sizeof(Option));

    for (i = 0; i < count; i++) {
        SDL_Texture* currentTexture = loadTexture(paths[i]);
        SDL_Rect currentSpriteClip = spriteRects[i];

        ret.options[i].texture = currentTexture;
        ret.options[i].spriteRect = currentSpriteClip;
    }

    if (cursorTexture == NULL)
        cursorTexture = loadTexture("resource/hud.gif");

    return ret;
}

void advanceMenu(Menu* menu, int step) {
    menu->currentOption = (menu->currentOption + step) % menu->countOptions;
    if (menu->currentOption < 0) {
        SDL_Log("Teste %d %d", menu->countOptions + menu->currentOption,
                menu->currentOption);
        menu->currentOption = menu->countOptions - 1;
    }
}

static void drawTextMenu(SDL_Renderer* renderer, Menu* menu) {
    static const SDL_Color textColor = {0, 0, 0};
    SDL_Rect currentTransform = {.x = WIDTH_SMALL_SPRITE / 2,
                                 .y = HEIGHT_BUTTON + HEIGHT_SMALL_SPRITE / 2,
                                 .w = WIDTH_SCREEN / 2,
                                 .h = HEIGHT_SMALL_SPRITE / 2};
    SDL_Rect cursorTransform = {
        .x = currentTransform.x - WIDTH_SMALL_SPRITE / 4,
        .y = (currentTransform.y - HEIGHT_SMALL_SPRITE / 12),
        .w = WIDTH_SMALL_SPRITE / 2,
        .h = HEIGHT_SMALL_SPRITE / 2};
    if (menu->currentOption > 0 && menu->currentOption % 2 != 0)
        cursorTransform.y += currentTransform.h;

    int index = (menu->currentOption / 2) * 2;
    const int parada = index + 2 >= menu->countOptions ? index + 1 : index + 2;
    SDL_Log("%d %d", index, parada);
    for (; index != parada; index++) {
        SDL_Texture* currentText =
            createTextTexture(textColor, "%s", menu->options[index].text);
        if (currentText == NULL)
            continue;

        SDL_RenderCopy(renderer, currentText, NULL, &currentTransform);
        SDL_DestroyTexture(currentText);

        currentTransform.y += currentTransform.h;
    }

    SDL_RenderCopy(renderer, cursorTexture, &cursorClip, &cursorTransform);
}

static void drawImageMenu(SDL_Renderer* renderer, Menu* menu) {
    static const SDL_Rect transform = {.x = WIDTH_SCREEN / 2 - WIDTH_SPRITE / 2,
                                       .y = HEIGHT_BUTTON,
                                       .w = WIDTH_SPRITE,
                                       .h = HEIGHT_SPRITE + STEP_SPRITE};
    static const SDL_Point centerCursor = {.x = 0, .y = 4};

    const Option* currentOption = &menu->options[menu->currentOption];
    SDL_RenderCopy(renderer, currentOption->texture, &currentOption->spriteRect,
                   &transform);

    SDL_Rect transformCursor = transform;
    transformCursor.x = transform.x + (WIDTH_SPRITE / 2 + WIDTH_SMALL_SPRITE +
                                       WIDTH_SMALL_SPRITE / 2);
    SDL_RenderCopy(renderer, cursorTexture, &cursorClip, &transformCursor);
    transformCursor.x = transform.x - (WIDTH_SPRITE / 2 + WIDTH_SMALL_SPRITE +
                                       WIDTH_SMALL_SPRITE / 2);
    SDL_RenderCopyEx(renderer, cursorTexture, &cursorClip, &transformCursor,
                     0.f, NULL, SDL_FLIP_HORIZONTAL);
}

void drawMenu(SDL_Renderer* renderer, Menu* menu) {
    if (menu->type == TEXT)
        drawTextMenu(renderer, menu);
    else
        drawImageMenu(renderer, menu);
}

void freeMenu(Menu* menu) {
    if (menu->type == IMAGE) {
        int i;
        for (i = 0; i < menu->countOptions; i++) {
            freeTexture(menu->options[i].texture);
            menu->options[i].texture = NULL;
        }
    }

    free(menu->options);
    memset(menu, 0, sizeof(Menu));
}