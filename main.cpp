#include <SDL.h>
#include "World.h"
#include "TestManager.h"
#include "PrecomputedData.h"
#include "Move.h"
#include "AI.h"
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
-/O2; /Ob2; whatever(i use /Oi); /Ot; wahtever(i use No(/Oy-)); wahtever(i use No); /GL
make sure to compile on x64
TODO:
    -add move ordering and zobrist transposition table
    -add another bool to template<> search for knowing if it is root, then to print(temp)/store best move
    -add some tests? (take them from chess.com puzzles) in TestMaganer
*/

#include <iostream>
int main(int argc, char* argv[]) {
    unsigned long long lastSecond = 0;
    unsigned long long lastTick = 0;
    unsigned long long ticksSinceSecond = 0;
    unsigned long long framesSinceSecond = 0;

    initData();
    Position::initLegalMoves();
    
    //Position* test = new Position();
    //test->readFEN("8/k7/7R/6R1/8/8/8/7K w - - 0 1");
    //AI testAI;
    //testAI.initPos(test);
    //std::cout << testAI.search<1>(4, -1000000, 1000000);

    //initTests();
    //runTests();
    //runPerft(7);

    //world.init();
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