#include "digihardware.h"

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

static uint16_t guiTime = 0xffff;

uint16_t DIGIHW_setTime() {
    time_t uiNow = time(NULL);
    struct tm* pstCurrentTime = localtime(&uiNow);

    guiTime = (uint16_t)(pstCurrentTime->tm_hour * 60 + pstCurrentTime->tm_min);
    return guiTime;
}

uint16_t DIGIHW_timeMinutes() {
    return guiTime;
}

void DIGIHW_addTime(uint16_t uiAddingAmount) {
    guiTime = (guiTime + uiAddingAmount) % 1440;
}

int16_t DIGIHW_readFile(const char* szFileName, void* pbDest,
                        const uint16_t uiMaxDestSize) {
    FILE* pstFileHandle = fopen(szFileName, "rb");
    if (pstFileHandle == NULL)
        return -1;

    const uint16_t iBytesRead = fread(pbDest, 1, uiMaxDestSize, pstFileHandle);
    fclose(pstFileHandle);
    return iBytesRead;
}

uint16_t DIGIHW_saveFile(const char* szFileName, const void* pbData,
                         const uint16_t uiSizeData) {
    FILE* pstFileHandle = fopen(szFileName, "wb");
    if (pstFileHandle == NULL)
        return -1;

    const uint16_t iBytesRead = fwrite(pbData, uiSizeData, 1, pstFileHandle);
    fclose(pstFileHandle);
    return iBytesRead;
}

uint8_t DIGIHW_randomNumber() {
    srand(time(NULL));

    return rand() % 16 + 1;
}
