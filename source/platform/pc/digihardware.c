#include "digihardware.h"

#include <stdio.h>
#include <time.h>

uint32_t DIGIHW_timeSeconds() {
    return time(NULL);
}

int16_t DIGIHW_readFile(const char* szFileName, void* pbDest,
                        const uint16_t uiMaxDestSize) {
    FILE* pstFileHandle = fopen(szFileName, "rb");
    if (pstFileHandle == NULL)
        return -1;

    const uint16_t iBytesRead = fread(pbDest, uiMaxDestSize, 1, pstFileHandle);
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
