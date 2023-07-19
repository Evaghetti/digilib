#ifndef DIGIBATTLE_CLASSIC_H
#define DIGIBATTLE_CLASSIC_H

#include "digitype.h"

#include "digimon.h"

#define DIGIBATTLE_RET_OK    0
#define DIGIBATTLE_RET_WIN   (DIGIBATTLE_RET_OK + 1)
#define DIGIBATTLE_RET_LOSE  (DIGIBATTLE_RET_OK + 2)
#define DIGIBATTLE_RET_ERROR (DIGIBATTLE_RET_OK + 4)
#define DIGIBATTLE_RET_POLL  (DIGIBATTLE_RET_OK + 5)

uint8_t DIGI_battle(playing_digimon_t* pstPlayingDigimon, uint8_t uiInitiate);

uint8_t DIGIBATTLE_initiate(playing_digimon_t* pstPlayingDigimon);

uint8_t DIGIBATTLE_continue(playing_digimon_t* pstPlayingDigimon);

uint8_t DIGIBATTLE_canBattle(const playing_digimon_t* pstPlayingDigimon);

uint16_t DIGIBATTLE_createFirstPacket(
    const playing_digimon_t* pstPlayingDigimon);

uint16_t DIGIBATTLE_createSecondPacket(
    const playing_digimon_t* pstPlayingDigimon, uint8_t uiResult);

uint8_t DIGIBATTLE_getBattleResult(uint8_t uiMySlot, uint8_t uiEnemySlot);

void DIGIBATTLE_changeStats(playing_digimon_t* pstPlayingDigimon,
                            uint8_t uiResultBattle);

#endif
