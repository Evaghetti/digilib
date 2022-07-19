#include "digivice/menu.h"

#include "digivice/globals.h"
#include "digivice/texture.h"

#include "SDL2/SDL_image.h"

#include <stdarg.h>

static SDL_Texture* cursorTexture = NULL;
static const SDL_Rect cursorClip = {.x = 32, .y = 0, .w = 8, .h = 8};

Menu initMenu(int count, TypeMenu type) {
    Menu ret = {.countOptions = count,
                .type = type,
                .currentOption = 0,
                .customs = NONE};

    ret.options = calloc(count, sizeof(Option));

    if (cursorTexture == NULL)
        cursorTexture = loadTexture("resource/hud.png");
    return ret;
}

Menu initMenuText(int count, char* texts[]) {
    Menu ret = {.countOptions = count,
                .type = TEXT,
                .currentOption = 0,
                .customs = NONE};
    int i;

    ret.options = calloc(count, sizeof(Option));

    for (i = 0; i < count; i++) {
        const char* currentArg = texts[i];

        strncpy(ret.options[i].text, currentArg, sizeof(ret.options[i].text));
    }

    if (cursorTexture == NULL)
        cursorTexture = loadTexture("resource/hud.png");

    return ret;
}

Menu initMenuImage(int count, char* paths[], SDL_Rect spriteRects[]) {
    Menu ret = {.countOptions = count,
                .type = IMAGE,
                .currentOption = 0,
                .customs = NONE};
    int i;

    ret.options = calloc(count, sizeof(Option));

    for (i = 0; i < count; i++)
        addMenuImage(&ret, paths[i], spriteRects[i]);

    if (cursorTexture == NULL)
        cursorTexture = loadTexture("resource/hud.png");

    return ret;
}

void addMenuImage(Menu* menu, const char* path, SDL_Rect spriteRect) {
    Option* currentOption = &menu->options[0];
    int i;

    for (i = 0; i < menu->countOptions; i++) {
        if (currentOption->texture == NULL)
            break;
        currentOption++;
    }

    currentOption->texture = loadTexture(path);
    currentOption->spriteRect = spriteRect;
}

Menu initMenuImageRaw(int count, SDL_Texture* textures[]) {
    Menu ret = {.countOptions = count,
                .type = IMAGE,
                .currentOption = 0,
                .customs = NONE};
    int i;

    ret.options = calloc(count, sizeof(Option));

    for (i = 0; i < count; i++) {
        SDL_Texture* currentTexture = textures[i];
        int width, height;
        SDL_QueryTexture(currentTexture, NULL, NULL, &width, &height);

        const SDL_Rect spriteRect = {0, 0, width, height};
        ret.options[i].texture = currentTexture;
        ret.options[i].spriteRect = spriteRect;
    }

    if (cursorTexture == NULL)
        cursorTexture = loadTexture("resource/hud.png");
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

static void drawNormalTextMenu(SDL_Renderer* renderer, Menu* menu) {
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
    const int parada = index + 2 >= menu->countOptions && menu->countOptions > 2
                           ? index + 1
                           : index + 2;

    for (; index != parada; index++) {
        SDL_Texture* currentText =
            createTextTexture(textColor, "%s", menu->options[index].text);
        if (currentText == NULL)
            continue;

        SDL_RenderCopy(renderer, currentText, NULL, &currentTransform);
        SDL_DestroyTexture(currentText);

        currentTransform.y += currentTransform.h;
    }

    if ((menu->customs & NO_CURSOR) == 0)
        SDL_RenderCopy(renderer, cursorTexture, &cursorClip, &cursorTransform);
}

static void drawHeaderTextMenu(SDL_Renderer* renderer, Menu* menu) {
    static const SDL_Color textColor = {0, 0, 0};
    SDL_Rect currentTransform = {0, HEIGHT_BUTTON, WIDTH_SCREEN,
                                 HEIGHT_SMALL_SPRITE};

    SDL_RenderCopy(renderer, menu->header, NULL, &currentTransform);
    currentTransform.y += currentTransform.h;
    currentTransform.w *= .25f;
    currentTransform.x += currentTransform.w * .5f;

    int index = (menu->currentOption / 2) * 2;
    const int parada = index + 2 >= menu->countOptions && menu->countOptions > 2
                           ? index + 1
                           : index + 2;

    for (; index != parada; index++) {
        SDL_Texture* currentText =
            createTextTexture(textColor, "%s", menu->options[index].text);
        if (currentText == NULL)
            continue;

        SDL_RenderCopy(renderer, currentText, NULL, &currentTransform);
        SDL_DestroyTexture(currentText);

        if (index == menu->currentOption && (menu->customs & NO_CURSOR) == 0) {
            SDL_Rect cursorTransform = currentTransform;
            cursorTransform.x -= currentTransform.w / 2;

            SDL_RenderCopy(renderer, cursorTexture, &cursorClip,
                           &cursorTransform);
        }

        currentTransform.x += currentTransform.w * 2.f;
    }
}

static void drawTextMenu(SDL_Renderer* renderer, Menu* menu) {
    if (menu->header == NULL) {
        drawNormalTextMenu(renderer, menu);
    } else {
        drawHeaderTextMenu(renderer, menu);
    }
}

static void drawImageMenu(SDL_Renderer* renderer, Menu* menu) {
    SDL_Rect transform = {.y = HEIGHT_BUTTON, .h = HEIGHT_SPRITE};
    if (menu->customs & FILL_SCREEN) {
        transform.x = 0;
        transform.w = WIDTH_SCREEN;
    } else {
        transform.x = WIDTH_SCREEN / 2 - WIDTH_SPRITE / 2;
        transform.w = WIDTH_SPRITE;
    }

    const Option* currentOption = &menu->options[menu->currentOption];
    SDL_RenderCopy(renderer, currentOption->texture, &currentOption->spriteRect,
                   &transform);

    if ((menu->customs & NO_CURSOR) == 0) {
        SDL_Rect transformCursor = transform;
        transformCursor.x =
            transform.x +
            (WIDTH_SPRITE / 2 + WIDTH_SMALL_SPRITE + WIDTH_SMALL_SPRITE / 2);
        SDL_RenderCopy(renderer, cursorTexture, &cursorClip, &transformCursor);
        transformCursor.x =
            transform.x -
            (WIDTH_SPRITE / 2 + WIDTH_SMALL_SPRITE + WIDTH_SMALL_SPRITE / 2);
        SDL_RenderCopyEx(renderer, cursorTexture, &cursorClip, &transformCursor,
                         0.f, NULL, SDL_FLIP_HORIZONTAL);
    }
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
    if (menu->header)
        freeTexture(menu->header);
    memset(menu, 0, sizeof(Menu));
}