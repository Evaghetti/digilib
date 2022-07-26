#ifndef ANDROID_H
#define ANDROID_H

#include "jni.h"

#ifdef __cplusplus
extern "C" {
#endif

void Java_org_libsdl_app_BackgroundUpdate_callUpdate(
        JNIEnv *env, jclass obj);

#ifdef __cplusplus
}
#endif
#endif