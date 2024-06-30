#pragma once
#include "Position.h"
#include <cinttypes>
using std::int8_t;

class PerftTest {
public:
	PerftTest() { 
		depth = 0; 
		nodesCount = 0; 
		fen = nullptr; 
	}
	PerftTest(int _depth, long long _nodesCnt, char* _fen) { 
		depth = _depth; 
		nodesCount = _nodesCnt; 
		fen = _fen; 
	}
	~PerftTest() {
		if (fen != nullptr) {
			delete[] fen;
		}
	}
	void init(int _depth, long long _nodesCnt, char* _fen) {
		depth = _depth;
		nodesCount = _nodesCnt;
		fen = _fen;
	}
	
	int depth;
	long long nodesCount;
	char* fen;
};

extern int8_t testsCnt;
extern PerftTest perftTests[100];

extern void initTests();
extern void runTests();
extern void runDebuggingTest(Position* worldPos = nullptr);
extern long long perft(Position& pos, int depth, bool maxDepth = 1);
extern void runPerft(int depth);