
#include "digivice/game.h"

#include <SDL_main.h>

int main(int argc, char* argv[]) {
    if (!initGame())
        return 1;

    while (updateGame()) {
        drawGame();
    }

    cleanUpGame();
    return 0;
}