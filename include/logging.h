#ifndef LOGGING_H
#define LOGGING_H

#ifndef NDEBUG

#include <stdarg.h>

#define LOG(fmt, ...)                                                   \
    addLog("[DIGILIB@%s:%d - %s/%s] " fmt "\n", __FUNCTION__, __LINE__, \
           getSaveFile(), getTime(), ##__VA_ARGS__)

void addLog(const char* szFmt, ...);

const char* getTime();

const char* getSaveFile();

#else

#define LOG(fmt, ...)

#endif

#endif