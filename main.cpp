#include <SDL.h>
#include "World.h"
#include "TestManager.h"
#include "PrecomputedData.h"
using std::cin;
using std::cout;
using std::endl;

World world;
/*
IF DEBUGGING:
- change "Whole program optimization" to "NONE"
- change "Optimization" to "/Od"
- change "Basic runtime checks" to "Both"
- change "Debug information format" to "/Zl"
MAXIMUM OPTIMISATIONS IS:
-/O2; /Ob2; whatever(i use /Oi); /Ot; wahtever(i use No(/Oy-)); wahtever(i use No); /GL
make sure to compile on x64
TODO:
    -fix
    .if spam queens, they will be more valuable than mate, so do something like max(mateScore/2, pcsEval)?
    .fix bot always drawing by repetition

    -change
    .make more evaluations to ordering

    -add
    .add bonuses for sq
    .add draws by insufficient material and repetition of moves
    .add some tests? (take them from chess.com puzzles) in TestMaganer

    -read
    .<https://www.chessprogramming.org/CPW-Engine_search>
    .<https://www.chessprogramming.org/Pruning>
    .<https://www.chessprogramming.org/Search>

    .<https://www.chessprogramming.org/ProbCut>
    .<https://www.chessprogramming.org/Delta_Pruning>
    .<https://www.chessprogramming.org/Futility_Pruning>
    .<https://www.chessprogramming.org/Reverse_Futility_Pruning>
    .<https://www.chessprogramming.org/Lazy_Evaluation>
    .<https://www.chessprogramming.org/Lazy_SMP>
    .<https://www.chessprogramming.org/Scout>
*/
#include "AI.h"
#include <iostream>

int main(int argc, char* argv[]) {
    unsigned long long lastSecond = 0;
    unsigned long long lastTick = 0;
    unsigned long long ticksSinceSecond = 0;
    unsigned long long framesSinceSecond = 0;

    initData();
    Position::initLegalMoves();
    tt.setSize(16);
    initTests();

    Position* pos = new Position();
    char fen[] = "8/3KP3/8/8/8/8/8/6kq b - - 0 1";
    //char fen[] = "8/3r4/3k4/8/8/3K4/8/8 b - - 0 1";
    //char fen[] = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";
    //reverseFenPosition(fen);
    pos->readFEN(fen);

    world.init();
    world.initPos(pos);
    //runDebuggingTest(pos);
    //runPerft(*pos, 7);
    //runTests();

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