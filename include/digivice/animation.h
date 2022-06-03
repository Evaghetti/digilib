#ifndef ANIMATION_H
#define ANIMATION_H

#include <stdarg.h>
#include "SDL2/SDL_rect.h"

typedef struct Frame {
    SDL_Rect frameClip;
    float timeHoldSecs;
    struct Frame* nextFrame;
} Frame;

typedef struct Animation {
    Frame* firstFrame;
    Frame* currentFrame;
    const char* animationName;
} Animation;

typedef struct AnimationController {
    Animation* animations;
    float timeInCurrentFrame;
    int animationCount, currentAnimation;
} AnimationController;

void addAnimation(AnimationController* animationController,
                  const char* animationName, int frameCount, ...);

void setCurrentAnimation(AnimationController* animationController,
                         const char* animationName);

void updateAnimation(AnimationController* animationController, float deltaTime);

const SDL_Rect* getAnimationFrameClip(
    const AnimationController* animationController);

void freeAnimationController(AnimationController* animationController);

SDL_Rect createRect(int x, int y, int w, int h);

#endif