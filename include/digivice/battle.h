#ifndef BATTLE_H
#define BATTLE_H

#include "digimon.h"
#include "digivice/menu.h"

typedef enum {
    ERROR = 0,
    WIN = 1 << 0,
    LOSE = 1 << 1,
    ERROR_DIGILIB = 1 << 2,
    HANDLING_MENU = 1 << 3,
    NOTHING_HAPPENED = 1 << 4,
} StatusUpdate;

int connectToServer();

int registerUser(digimon_t* digimon);

StatusUpdate updateClient(Menu* menu, int selectedOption);

int challengeUser(int offsetUser);

int disconnectFromServer();

#endif  // BATTLE_H