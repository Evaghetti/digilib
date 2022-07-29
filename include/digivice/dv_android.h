#ifndef ANDROID_H
#define ANDROID_H

#include "jni.h"

#ifdef __cplusplus
extern "C" {
#endif

jint Java_org_libsdl_app_BackgroundUpdate_callUpdate(JNIEnv* env, jclass obj, jint deltaTime);

#ifdef __cplusplus
}
#endif
#endif