#include "digivice/dv_android.h"

#include "digivice/game.h"

jint Java_org_libsdl_app_BackgroundUpdate_callUpdate(JNIEnv* env, jclass obj, jint deltaTime) {
    return updateBackGround(deltaTime);
}