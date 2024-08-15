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
- change "Support just my code debugging" to Yes(/JMC)
MAXIMUM OPTIMISATIONS IS:
-/O2; /Ob2; whatever(i use /Oi); /Ot; wahtever(i use Yes(/Oy)); wahtever(i use /GT); /GL
make sure to compile on x64
TODO:
    -fix
    .if spam queens, they will be more valuable than mate, so do something like max(mateScore/2, pcsEval)?
    .fix bot always drawing by repetition
    ..this might be, because: Eval position A; make best move; human makes move;
      When evaluating position B, will find a way to go back to position A and will get
      a better evaluation or if it is equal, will dismiss any other moves

    -add
    .timer
    .add draws by insufficient material and repetition of moves
    .make more evaluations to ordering

    -maybe
    .change transition from midgame bonuses to endgame bonuses to sigmoid or tanh
    .add some tests? (take them from chess.com puzzles) in TestMaganer

    -read
    .<https://www.chessprogramming.org/CPW-Engine_search>
    .<https://www.chessprogramming.org/Pruning>
    .<https://www.chessprogramming.org/Search>
    .all links in <https://www.chessprogramming.org/Evaluation>

    .<https://www.chessprogramming.org/Multi-Cut>
    .<https://www.chessprogramming.org/Uncertainty_Cut-Offs>

    .<https://www.chessprogramming.org/ProbCut>

    .<https://www.chessprogramming.org/Lazy_Evaluation>
    .<https://www.chessprogramming.org/Lazy_SMP>
*/
#include "AI.h"
#include <iostream>
int main(int argc, char* argv[]) {
    unsigned long long lastSecond = 0;
    unsigned long long lastTick = 0;
    unsigned long long ticksSinceSecond = 0;
    unsigned long long framesSinceSecond = 0;

    initData();
    tt.setSize(64);
    initTests();
    Position* pos = new Position();
    //char fen[] = "8/3KP3/8/8/8/8/8/6kq b - - 0 1";
    //char fen[] = "8/3r4/3k4/8/8/3K4/8/8 b - - 0 1";
    //char fen[] = "8/k7/3p4/p2P1p2/P2P1P2/8/8/K7 w - - 0 1";
    //char fen[] = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";
    char fen[] = "r1bqk2r/ppp2ppp/2np1n2/2b1p3/2B1P3/2NP1N2/PPP2PPP/R1BQK2R w KQkq - 0 1";
    //char fen[] = "r2qk2r/1p3p1p/p1pb1np1/n2p1p2/Q2P4/2N1PN2/PPP2PPP/R1B2RK1 w KQkq - 0 1";
    //reverseFenPosition(fen);
    pos->readFEN(fen);

    world.init();
    world.initPos(pos);
    //runDebuggingTest(pos);
    //runPerft(*pos, 7);
    //runTests();

    while (1) {
        // new second starts
        if (lastSecond + 1000 <= SDL_GetTicks64()) {
            lastSecond = SDL_GetTicks64() / 1000 * 1000;

            //cout << "| TPS: " << ticksSinceSecond << endl;
            //cout << "| FPS: " << framesSinceSecond << endl;
            //cout << endl;

            ticksSinceSecond = 0;
            framesSinceSecond = 0;
        }

        // tick
        if (ticksSinceSecond != 120 && ticksSinceSecond * 1000 / 120 <= SDL_GetTicks64() - lastSecond) {
            lastTick = SDL_GetTicks64();

            // tick functions
            world.update();
            ticksSinceSecond++;
        }

        // quit
        if (world.quit) {
            break;
        }

        // frame
        if (framesSinceSecond != 512 && framesSinceSecond * 1000 / 512 <= SDL_GetTicks64() - lastSecond) {
            world.draw();

            framesSinceSecond++;
        }
    }
    SDL_Quit();
    return 0;
}