#include "digivice_hal.h"

#include "enums_digivice.h"

const digivice_hal_t* gpstDigiviceHal;

uint8_t DIGIVICE_saveData(const void* pData, size_t size) {
    if (gpstDigiviceHal->saveData == NULL)
        return DIGIVICE_RET_ERROR;

    return gpstDigiviceHal->saveData(pData, size) != size ? DIGIVICE_RET_ERROR
                                                          : DIGIVICE_RET_OK;
}

uint8_t DIGIVICE_readData(void* pData, size_t size) {
    if (gpstDigiviceHal->readData == NULL)
        return DIGIVICE_RET_ERROR;

    return gpstDigiviceHal->readData(pData, size) != size ? DIGIVICE_RET_ERROR
                                                          : DIGIVICE_RET_OK;
}
