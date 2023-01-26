#include "digihal.h"

#include "enums.h"

#include <stdio.h>

digihal_t gstHal;

uint8_t DIGI_setHal(digihal_t stConfig) {
    uint8_t iRet = DIGI_RET_OK;

    if (stConfig.malloc == NULL)
        iRet = DIGI_RET_ERROR;
    else if (stConfig.free == NULL)
        iRet = DIGI_RET_ERROR;
    else if (stConfig.randomNumber == NULL)
        iRet = DIGI_RET_ERROR;

    if (stConfig.getTimeStamp == NULL)
        iRet = DIGI_RET_ERROR;

    if (iRet == DIGI_RET_OK) {
        gstHal = stConfig;
    }
    return iRet;
}