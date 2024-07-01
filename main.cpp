#include <SDL.h>
#include "World.h"
#include "TestManager.h"
#include "PrecomputedData.h"
#include "Move.h"
#include <iostream>
using std::cin;
using std::cout;
using std::endl;

World world;
/*
IF DEBUGGING:
- change "Whole program optimization" to "NONE"
- change "Optimisation" to "/Od"
- change "Basic runtime checks" to "Both"
- change "Debug information format" to "/Zl"
MAXIMUM OPTIMISATIONS IS:
-/O2; /Ob2; unchange(whatever); /Ot; unchange(wahtever); unchange(wahtever); /GL
make sure to compile on x64
TODO:
    -optimize
    .for loop in Position::makeMove when capturing might be slow (could store index inside pieces)
    .move generation is 25s for perft 7, while stockfish's is 15s :(
*/

//Current pos depth 6 is incorrect
int main(int argc, char* argv[]) {
    unsigned long long lastSecond = 0;
    unsigned long long lastTick = 0;
    unsigned long long ticksSinceSecond = 0;
    unsigned long long framesSinceSecond = 0;

    initData();
    Position::initLegalMoves();

    //initTests();
    //runTests();
    runPerft(7);

    world.init();
    //runDebuggingTest(&world.m_board.m_pos);

    while (1) {
        // new second starts
        if (lastSecond + 1000 <= SDL_GetTicks()) {
            lastSecond = SDL_GetTicks() / 1000 * 1000;

            //cout << "| TPS: " << ticksSinceSecond << endl;
            //cout << "| FPS: " << framesSinceSecond << endl;
            //cout << endl;

            ticksSinceSecond = 0;
            framesSinceSecond = 0;
        }

        // tick
        if (ticksSinceSecond != 120 && ticksSinceSecond * 1000 / 120 <= SDL_GetTicks() - lastSecond) {
            lastTick = SDL_GetTicks();

            // tick functions
            world.update();
            ticksSinceSecond++;
        }

        // quit
        if (world.quit) {
            break;
        }

        // frame
        if (framesSinceSecond != 512 && framesSinceSecond * 1000 / 512 <= SDL_GetTicks() - lastSecond) {
            world.draw();

            framesSinceSecond++;
        }
    }
    SDL_Quit();
    return 0;
}