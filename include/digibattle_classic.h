#ifndef DIGIBATTLE_CLASSIC_H
#define DIGIBATTLE_CLASSIC_H

#include <stdint.h>

#define DIGIBATTLE_RET_OK    0
#define DIGIBATTLE_RET_WIN   (DIGIBATTLE_RET_OK + 1)
#define DIGIBATTLE_RET_LOSE  (DIGIBATTLE_RET_OK + 2)
#define DIGIBATTLE_RET_ERROR (DIGIBATTLE_RET_OK + 3)

uint8_t DIGI_battle(uint8_t uiInitiate);

uint8_t DIGIBATTLE_initiate();

uint8_t DIGIBATTLE_continue();

uint8_t DIGIBATTLE_canBattle();

uint16_t DIGIBATTLE_createFirstPacket();

uint16_t DIGIBATTLE_createSecondPacket(uint8_t uiResult);

uint8_t DIGIBATTLE_getBattleResult(uint8_t uiMySlot, uint8_t uiEnemySlot);

void DIGIBATTLE_changeStats(uint8_t uiResultBattle);

#endif