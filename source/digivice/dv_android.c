#include "digivice/dv_android.h"

#include "digivice/game.h"

void Java_org_libsdl_app_BackgroundUpdate_callUpdate(
    JNIEnv* env, jclass obj) {
    updateBackGround();
}