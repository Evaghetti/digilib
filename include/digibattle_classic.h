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

#endif