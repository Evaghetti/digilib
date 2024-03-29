#ifndef ANIMATION_H
#define ANIMATION_H

#include <SDL_rect.h>
#include <stdarg.h>

typedef struct Frame {
    SDL_Rect frameClip;
    float timeHoldSecs;
    struct Frame* nextFrame;
} Frame;

typedef struct Animation {
    Frame* firstFrame;
    Frame* currentFrame;
    const char* animationName;
    int finished;
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

void resetCurrentAnimation(AnimationController* animationController);

const SDL_Rect* getAnimationFrameClip(
    const AnimationController* animationController);

int finishedCurrentAnimation(AnimationController* animationController);

void markAnimationAsFinished(AnimationController* animationController);

int isCurrentAnimation(AnimationController* animationController,
                       const char* name);

int isCurrentAnimationAndFinished(AnimationController* animationController,
                                  const char* name);

int isFirstFrame(const AnimationController* animationController);

void freeAnimationController(AnimationController* animationController);

SDL_Rect createRect(int x, int y, int w, int h);

#endif