#ifndef BATTLE_H
#define BATTLE_H

#include "digimon.h"
#include "digivice/menu.h"

int connectToServer();

int registerUser(digimon_t* digimon);

int updateClient(Menu* menu, int selectedOption, int* resultBattle);

int challengeUser(int offsetUser);

int disconnectFromServer();

#endif  // BATTLE_H