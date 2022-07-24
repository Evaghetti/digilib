#include "animation.h"

#include <stdlib.h>
#include <string.h>

void addAnimation(AnimationController* animationController,
                  const char* animationName, int frameCount, ...) {
    animationController->animationCount++;
    animationController->animations =
        realloc(animationController->animations,
                sizeof(Animation) * animationController->animationCount);

    Animation* animation = animationController->animations +
                           (animationController->animationCount - 1);
    memset(animation, 0, sizeof(Animation));

    va_list vl;
    va_start(vl, frameCount);

    int i;
    for (i = 0; i < frameCount; i++) {
        Frame* newFrame = calloc(1, sizeof(Frame));
        newFrame->frameClip = va_arg(vl, SDL_Rect);
        newFrame->timeHoldSecs = va_arg(vl, double);

        if (animation->firstFrame == NULL) {
            animation->firstFrame = newFrame;
            animation->currentFrame = animation->firstFrame;
        } else {
            animation->currentFrame->nextFrame = newFrame;
            animation->currentFrame = animation->currentFrame->nextFrame;
        }
    }
    va_end(vl);

    animation->currentFrame->nextFrame = animation->firstFrame;
    animation->currentFrame = animation->firstFrame;
    animation->animationName = animationName;
}

void setCurrentAnimation(AnimationController* animationController,
                         const char* animationName) {
    Animation* currentAnimation =
        &animationController->animations[animationController->currentAnimation];
    int i;

    if (strcmp(currentAnimation->animationName, animationName) == 0) {
        return;
    }

    for (i = 0; i < animationController->animationCount; i++) {
        Animation* animation = &animationController->animations[i];

        if (strcmp(animation->animationName, animationName) == 0) {
            animation->currentFrame = animation->firstFrame;
            animation->finished = 0;

            animationController->currentAnimation = i;
            animationController->timeInCurrentFrame = 0.f;
            break;
        }
    }
}

void updateAnimation(AnimationController* animationController,
                     float deltaTime) {
    Animation* animation =
        &animationController->animations[animationController->currentAnimation];

    animationController->timeInCurrentFrame += deltaTime;
    if (animationController->timeInCurrentFrame >=
        animation->currentFrame->timeHoldSecs) {
        animation->currentFrame = animation->currentFrame->nextFrame;
        animationController->timeInCurrentFrame = 0.f;

        animation->finished = animation->currentFrame == animation->firstFrame;
    }
}

void resetCurrentAnimation(AnimationController* animationController) {
    animationController->animations[animationController->currentAnimation]
        .currentFrame =
        animationController->animations[animationController->currentAnimation]
            .firstFrame;
    animationController->animations[animationController->currentAnimation]
        .finished = 0;
    animationController->timeInCurrentFrame = 0.f;
}

const SDL_Rect* getAnimationFrameClip(
    const AnimationController* animationController) {
    return &animationController
                ->animations[animationController->currentAnimation]
                .currentFrame->frameClip;
}

int finishedCurrentAnimation(AnimationController* animationController) {
    const Animation* animation =
        &animationController->animations[animationController->currentAnimation];

    return animation->finished;
}

int isCurrentAnimation(AnimationController* animationController,
                       const char* name) {
    const char* currentName =
        animationController->animations[animationController->currentAnimation]
            .animationName;

    return strcmp(name, currentName) == 0;
}

int isCurrentAnimationAndFinished(AnimationController* animationController,
                                  const char* name) {
    return isCurrentAnimation(animationController, name) &&
           finishedCurrentAnimation(animationController);
}

int isFirstFrame(const AnimationController* animationController) {
    const Frame* currentFrame =
        animationController->animations[animationController->currentAnimation]
            .currentFrame;

    return currentFrame ==
           animationController
               ->animations[animationController->currentAnimation]
               .firstFrame;
}

void freeAnimationController(AnimationController* animationController) {
    int i;

    for (i = 0; i < animationController->animationCount; i++) {
        Animation* animation = animationController->animations + i;

        Frame* currentFrame = animation->firstFrame->nextFrame;
        while (currentFrame != animation->firstFrame) {
            Frame* next = currentFrame->nextFrame;
            free(currentFrame);
            currentFrame = next;
        }

        free(animation->firstFrame);
    }

    free(animationController->animations);
    memset(animationController, 0, sizeof(AnimationController));
}

SDL_Rect createRect(int x, int y, int w, int h) {
    SDL_Rect temp = {x, y, w, h};
    return temp;
}
