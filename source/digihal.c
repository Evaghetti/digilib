#include "digihal.h"

#include "enums.h"

#include <stdio.h>

const digihal_t* gpstHal;

uint8_t DIGI_setHal(const digihal_t* pstConfig) {
    uint8_t iRet = DIGI_RET_OK;

    if (pstConfig->malloc == NULL)
        iRet = DIGI_RET_ERROR;
    else if (pstConfig->free == NULL)
        iRet = DIGI_RET_ERROR;
    else if (pstConfig->randomNumber == NULL)
        iRet = DIGI_RET_ERROR;

    if (iRet == DIGI_RET_OK) {
        gpstHal = pstConfig;
    }
    return iRet;
}