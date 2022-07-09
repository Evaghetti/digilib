#ifndef BATTLE_H
#define BATTLE_H

#include "digimon.h"
#include "digivice/menu.h"

int connectToServer();

int registerUser(digimon_t* digimon, Menu* menu);

int getBattleList(Menu* menu);

int disconnectFromServer();

#endif  // BATTLE_H