#include "Position.h"
#include "TestManager.h"
#include "Move.h"
#include <string>
#include <SDL.h>
#include <iostream>
#include <fstream>
using std::cout;
using std::ifstream;

int8_t testsCnt = 0;
PerftTest perftTests[100];

void initTests() {
	ifstream read;
	read.open("assets/tests/PerftTests/Full.txt");
    int depth;
    long long nodesCnt;
	while (!read.eof()) {
		char* fen = new char[128]();
        read >> depth >> nodesCnt >> std::ws;
        read.getline(fen, 128);
		perftTests[testsCnt].init(depth, nodesCnt, fen);
		testsCnt++;
	}
	read.close();
	return;
}

void runPreftTests() {
    int globalStTime = SDL_GetTicks();
	Position pos = Position();
    long long totalPositions = 0;
	for (int i = 0; i < testsCnt; i++) {
        int stTime = SDL_GetTicks();
        pos.readFEN(perftTests[i].fen);
        long long res = perft(pos, perftTests[i].depth, 0);
        totalPositions += res;
        if (res == perftTests[i].nodesCount) {
            std::cout << "Test " << i << " passed. Time: " << SDL_GetTicks() - stTime << "\n";
        } else {
            std::cout << "Test " << i << " FAILED. Expected: " << perftTests[i].nodesCount << "; Result: " << res << "; Time: " << SDL_GetTicks() - stTime << "\n";
        }
	}
    cout << "Total time: " << SDL_GetTicks() - globalStTime << "\nSpeed: " << totalPositions / (SDL_GetTicks() - globalStTime) << " leaves/ms\n";
}

long long perft(Position& pos, int depth, bool root) {
    long long ans = 0;
    int8_t bitmaskCastling = pos.m_bitmaskCastling;
    int8_t possibleEnPassant = pos.m_possibleEnPassant;
    int16_t moves[256];
    pos.updateLegalMoves<0>(moves);
    if (depth == 1) {
        return pos.m_legalMovesCnt;
    }
    int16_t cnt = pos.m_legalMovesCnt;

    for (int16_t i = 0; i < cnt; i++) {
        if (root) {
            printName(moves[i]);
            cout << ": ";
        }
        int8_t capturedPieceIdx = pos.makeMove(moves[i]);
        long long increase = perft(pos, depth - 1, 0);
        if (root) {
            cout << increase << "\n";
        }
        ans += increase;
        pos.undoMove(moves[i], capturedPieceIdx, bitmaskCastling, possibleEnPassant, 0);
    }
    pos.m_legalMovesCnt = cnt;
    return ans;
}

void runDebuggingPerft(Position* pos) {
    ifstream read;
    read.open("assets/tests/PerftTests/DebuggingTest.txt");
    int depth;
    long long stTime = clock();
    char* fen = new char[128]();
    read >> depth >> std::ws;
    read.getline(fen, 128);
    if (pos != nullptr) {
        pos->readFEN(fen);
    }
    cout << "Total: " << perft(*pos, depth);
    cout << "; Time: " << clock() - stTime << "\n";
}

void runPerft(Position& pos, int depth) {
    int curTime = SDL_GetTicks();
    long long res = perft(pos, depth);
    cout << "Nodes count: " << res;
    cout << "\nTime: " << SDL_GetTicks() - curTime << "\nSpeed: " << res / (SDL_GetTicks() - curTime) << " leaves/ms\n";
}

#include "AI.h"
#include "AICompare.h"

void compareAI(World& world, Position* pos, int numberOfGames, int timeToMove) {
	int wins = 0, draws = 0, loses = 0;
	std::ifstream inputS;
	std::ofstream out("assets/tests/Results.txt");
	inputS.open("assets/tests/TestPositions/TestPositions.txt");
	RandNumGen rng(Clock::now());
	string fen = "";
	AI aiGood;
	AICompare aiBad;
	uint64_t repHistory[1024];
	int16_t stalemates = 0, draw50 = 0, drawInsuf = 0, drawRep = 0;
	bool twice[1024], flipped = 0;
	uint64_t stTime = Clock::now();
	for (int c = 0; c < numberOfGames; c++) {
		tt.clear();
		aiBad.ttCmp.clear();
		int repHistorySz = 0;
		for (int i = 0; i < 1024; i++) {
			repHistory[i] = twice[i] = 0;
		}
		int skipForward = rng.rand() % 10;
		for (int i = 0; i <= skipForward; i++) {
			getline(inputS, fen);
			inputS >> std::ws;
		}
		pos->readFEN(fen.c_str());
		world.initPos(pos);

		aiGood.initPos(pos);
		aiBad.initPos(pos);

		bool goodToMove = rng.rand() & 1;
		if ((goodToMove == pos->m_blackToMove) != flipped) {
			world.m_gameManager.m_board.flip(true);
			flipped = !flipped;
		}
		int move50count = 0;
		while (1) {
			if (goodToMove) {
				aiGood.bestMove = nullMove;
				aiGood.startSearch(timeToMove, 0);
				if (aiGood.bestMove == nullMove) {
					loses++;
					break;
				}
				move50count++;
				if (pos->isCapture(aiGood.bestMove) || pos->m_pieceOnTile[getStartPos(aiGood.bestMove)]) {
					move50count = 0;
				}
				//if (pos->isCapture(aiGood.bestMove)) {
				//	Nuke::nukes[getEndPos(aiGood.bestMove) / 8][getEndPos(aiGood.bestMove) % 8].activate();
				//}
				pos->makeMove(aiGood.bestMove);
			} else {
				aiBad.bestMove = nullMove;
				aiBad.startSearch(timeToMove, 0);
				if (aiBad.bestMove == nullMove) {
					wins++;
					break;
				}
				move50count++;
				if (pos->isCapture(aiBad.bestMove) || pos->m_pieceOnTile[getStartPos(aiBad.bestMove)]) {
					move50count = 0;
				}
				//if (pos->isCapture(aiBad.bestMove)) {
				//	Nuke::nukes[getEndPos(aiBad.bestMove) / 8][getEndPos(aiBad.bestMove) % 8].activate();
				//}
				pos->makeMove(aiBad.bestMove);
			}
			goodToMove = !goodToMove;

			if (pos->drawMan.checkForRep()) {
				drawRep++;
				draws++;
				break;
			}
			if (pos->drawMan.checkForRule50()) {
				draw50++;
				draws++;
				break;
			}
			pos->updateLegalMoves<0>(false);
			if (pos->m_legalMovesCnt == 0) {
				if (pos->friendlyInCheck) {
					if (goodToMove) {
						loses++;
					} else {
						wins++;
					}
				} else {
					stalemates++;
					draws++;
				}
				break;
			}
			if (((pos->m_whiteTotalPiecesCnt == 2 && (pos->m_whitePiecesCnt[BISHOP] == 1 || pos->m_whitePiecesCnt[KNIGHT] == 1)) ||
				pos->m_whiteTotalPiecesCnt == 1) &&
				((pos->m_blackTotalPiecesCnt == 2 && (pos->m_blackPiecesCnt[BISHOP] == 1 || pos->m_blackPiecesCnt[KNIGHT] == 1)) ||
				pos->m_blackTotalPiecesCnt == 1)) {
				drawInsuf++;
				draws++;
				break;
			}
			//world.draw();
			//world.m_input.getInput();
		}
		//Print
		system("cls");
		cout << wins + draws + loses << '\n';
		cout << "Wins:\n";
		for (int i = 0; i < wins; i++) { cout << '#'; }
		cout << "\nDraws by Rep:\n";
		for (int i = 0; i < drawRep; i++) { cout << '#'; }
		cout << "\nStalemates:\n";
		for (int i = 0; i < stalemates; i++) { cout << '#'; }
		cout << "\nDraws by 50 move rule:\n";
		for (int i = 0; i < draw50; i++) { cout << '#'; }
		cout << "\nDraws by insufficient material:\n";
		for (int i = 0; i < drawInsuf; i++) { cout << '#'; }
		cout << "\nLoses:\n";
		for (int i = 0; i < loses; i++) { cout << '#'; }
		int totalGames = wins + draws + loses;
		cout << "\nEstimated time:\n";
		uint64_t waitTime = ((Clock::now() - stTime) / totalGames) * (numberOfGames - totalGames);
		Clock::printTime(waitTime);
		//Used for backup if someting goes wrong
		out << wins << ' ' << draws << ' ' << loses << '\n';
	}
	std::cout << "\n\nWins: " << wins << "; Draws: " << draws << "; Loses: " << loses << '\n';
	delete pos;
}
