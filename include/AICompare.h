#pragma once
#include "AI.h"
#include "Position.h"
#include "Clock.h"
#include "misc.h"
#include <iostream>
#include <algorithm>
using std::max;
using std::min;

class AICompare {
public:
	AICompare();
	~AICompare();

	void initPos(Position* pos_);
	//@returns Eval
	int16_t startSearch(uint64_t timeToSearch, bool printRes = true);
	int16_t bestMove;

	TranspositionTable ttCmp;
private:
	int64_t searchEndTime;

	//Warning: pos is referece => changes made by the AI will impact the original position
	Position* pos;

	//@returns Eval
	int16_t iterativeDeepening(int8_t depth, bool printRes = true);
	//@returns Eval
	template<NodeType nodeType>
	int16_t search(int8_t depth, int16_t alpha, int16_t beta, bool cutNode);
	//@returns Eval
	int16_t searchOnlyCaptures(int16_t alpha, int16_t beta);
	inline void orderMoves(int16_t* moves, int16_t movesCnt, int16_t* indices,
		int16_t ttBestMove, int16_t bestMoveAfterNull, int16_t nmScoreDelta = 0);
	void resetHistory();
	void updateHistoryNewSearch();

	static const int MAX_PLY_MATE;
	//Used for ordering moves
	int* evalGuess;
	Stack<int16_t> movesHistory;
	static const int CHECK_BONUS;

	int16_t rootMovesEval[256];
	int16_t rootMoveIndices[256];

	int inScout;

	//<https://www.chessprogramming.org/History_Heuristic> 
	//Addressed by color, piece type, end square
	int historyHeuristic[2][6][64];
	static const int MAX_HISTORY;
	//<https://www.chessprogramming.org/History_Heuristic#Capture_History>
	//Addressed by color, piece type, end square, captured piece type
	int captureHistory[2][6][64][6];

	//<https://www.chessprogramming.org/Countermove_Heuristic>
	//Addressed by color, piece type, end square
	int16_t counterMove[2][6][64];
	static const int COUNTER_MOVE_BONUS;

	//<https://www.chessprogramming.org/Null_Move_Pruning>
	int inNullMoveSearch;
	static const int NULL_MOVE_DEFEND_BONUS;
	static const int NULL_MOVE_MATE_DEFEND_BONUS;

	//<https://www.chessprogramming.org/Killer_Heuristic>
	//Addressed by ply from root (second param is just to have 2 killers) 
	int16_t killers[128][2];
	static const int KILLER_BONUS;
	inline void updateKillers(int16_t newMove) {
		if (killers[movesHistory.size()][0] == nullMove) {
			killers[movesHistory.size()][0] = newMove;
		}
		else {
			if (killers[movesHistory.size()][1] != nullMove) {
				killers[movesHistory.size()][0] = killers[movesHistory.size()][1];
			}
			killers[movesHistory.size()][1] = newMove;
		}
	}

};