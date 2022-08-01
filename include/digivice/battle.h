#ifndef BATTLE_H
#define BATTLE_H

#include "digimon.h"
#include "digivice/menu.h"

#include <SDL_image.h>

typedef enum {
    ERROR = 0,
    WIN = 1 << 0,
    LOSE = 1 << 1,
    ERROR_DIGILIB = 1 << 2,
    HANDLING_MENU = 1 << 3,
    NOTHING_HAPPENED = 1 << 4,
} StatusUpdate;

int connectToServer(digimon_t* playerDigimon);

int registerUser(digimon_t* digimon);

StatusUpdate updateClient(Menu* menu, int selectedOption,
                          SDL_Renderer* renderer);

int challengeUser(int offsetUser);

int disconnectFromServer();

SDL_Texture* getChallengedUserTexture();

int updateSingleBattle(digimon_t* playerDigimon, Menu* menu,
                       int selectedOption);

void resetPlayers();

#endif  // BATTLE_H