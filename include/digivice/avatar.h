#ifndef AVATAR_H
#define AVATAR_H

#include "digivice/menu.h"

#include <SDL_image.h>
#include <SDL_rect.h>
#include <SDL_render.h>

#include "animation.h"
#include "battle.h"
#include "digimon.h"

typedef enum Action {
    HATCHING = 0,
    EVOLVING,
    WALKING = 1 << 1,
    EATING = 1 << 2,
    STRENGTHNING = 1 << 3,
    SLEEPING = 1 << 4,
    HEALING = 1 << 5,
    HAPPY = 1 << 6,
    HAPPY_TRAINING,
    HAPPY_SCORE,
    HAPPY_BATTLE,
    NEGATING = 1 << 7,
    CLEANING = 1 << 8,
    CLEANING_DEFEAT,
    TRAINING = 1 << 9,
    TRAINING_DOWN,
    TRAINING_UP,
    MAD = 1 << 10,
    MAD_TRAINING,
    MAD_SCORE,
    SAD_BATTLE,
    SHOWING_SCORE = 1 << 11,
    BATTLE_STATE = 1 << 12,
    BATTLE_WIN,
    BATTLE_LOSE,
    STANDOFF = 1 << 13,
} Action;

typedef struct {
    char name[256];

    SDL_Texture* spriteSheet;
    SDL_Rect transform;
    SDL_RendererFlip renderFlags;

    AnimationController animationController;
    Action currentAction;

    float timePassed;
    int secondsPassed;
    int initiated;
    int calling;
    int currentTraining;

    playing_digimon_t infoApi;
} Avatar;

int initAvatar(Avatar* ret, char* saveGame);

void updateAvatar(Avatar* avatar, const float deltaTime);

void drawAvatar(SDL_Renderer* render, const Avatar* avatar);

void handleEvents(Avatar* avatar, const unsigned char events);

void setCurrentAction(Avatar* avatar, Action newAction);

void setBattleAction(Avatar* avatar, StatusUpdate status, SDL_Texture* enemy);

Menu createTexturesInfoMenu(Avatar* avatar, SDL_Renderer* renderer);

void freeAvatar(Avatar* avatar);

#endif