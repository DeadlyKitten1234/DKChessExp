#include "TestManager.h"
#include "Move.h"
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

void runTests() {
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
    pos.updateLegalMoves<0>();
    if (depth == 1) {
        return pos.m_legalMovesCnt;
    }
    int16_t cnt = pos.m_legalMovesCnt;
    const int16_t* moves = pos.m_legalMoves + pos.m_legalMovesStartIdx;
    for (int16_t i = 0; i < cnt; i++) {
        if (root) {
            printName(moves[i]);
            cout << ": ";
        }
        int8_t capturedPieceIdx = pos.makeMove(moves[i]);
        pos.m_legalMovesStartIdx += cnt;
        long long increase = perft(pos, depth - 1, 0);
        if (root) {
            cout << increase << "\n";
        }
        ans += increase;
        pos.m_legalMovesStartIdx -= cnt;
        pos.undoMove(moves[i], capturedPieceIdx, bitmaskCastling, possibleEnPassant);
    }
    pos.m_legalMovesCnt = cnt;
    return ans;
}

void runDebuggingTest(Position* pos) {
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