#ifndef DIGIHAL_H
#define DIGIHAL_H

#include <stdarg.h>
#include "digitype.h"

#ifndef NDEBUG

#define LOG(x, ...)                        \
    if (gstHal.log) {                      \
        gstHal.log(x "\n", ##__VA_ARGS__); \
    }

#else

#define LOG(fmt, ...)

#endif  // NDGEBUG

typedef struct digihal_t {
    void* (*malloc)(size_t size);
    void (*free)(void* pBuff);
    int32_t (*log)(const char* buff, ...);

    size_t (*getTimeStamp)();
    int32_t (*saveData)(const void* pData, size_t size);
    int32_t (*readData)(void* pData, size_t size);
    size_t (*randomNumber)();
} digihal_t;

extern digihal_t gstHal;

uint8_t DIGI_setHal(digihal_t stHal);

#endif  // DIGIHAL_H