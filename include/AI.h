#pragma once
#include "TranspositionTable.h"
#include "Position.h"
#include "Clock.h"
#include "misc.h"
#include <iostream>
#include <xmmintrin.h>//_mm_prefetch
#include <algorithm>
using std::max;
using std::min;

enum NodeType : int8_t {
	//PV (principal variation(i think)) nodes are the 
	//nodes that are hit when players do the best moves
	PV = 0,
	//NonPv nodes can be cut-nodes (where alpha beta cutoff occured)
	//or all-nodes (where no score improved alpha)
	NonPV = 1,
	Root = 2
};

class AI {
public:
	AI();
	~AI();

	void initPos(Position* pos_);
	int16_t startSearch(uint64_t timeToSearch);
	int16_t bestMove;

private:
	int64_t searchEndTime;

	//Warning: pos is referece => changes made by the AI will impact the original position
	Position* pos;

	int16_t iterativeDeepening(int8_t depth);
	template<NodeType nodeType>
	int16_t search(int8_t depth, int16_t alpha, int16_t beta);
	int16_t searchOnlyCaptures(int16_t alpha, int16_t beta);
	inline void orderMoves	(int16_t moves[256], int16_t movesCnt, int16_t* indices, 
							int16_t ttBestMove, int16_t bestMoveAfterNull, bool mateIfNullMove = false);
	void resetHistory();
	void updateHistoryNewSearch();

	static const int MAX_PLY_MATE;
	//Used for ordering moves
	int* evalGuess;
	Stack<int16_t> movesHistory;
	static const int CHECK_BONUS;

	int inScout;

	//<https://www.chessprogramming.org/History_Heuristic>
	int historyHeuristic[2][6][64];
	static const int MAX_HISTORY;

	//<https://www.chessprogramming.org/Countermove_Heuristic>
	int16_t counterMove[2][6][64];
	static const int COUNTER_MOVE_BONUS;

	//<https://www.chessprogramming.org/Null_Move_Pruning>
	int inNullMoveSearch;
	static const int NULL_MOVE_DEFEND_BONUS;
	static const int NULL_MOVE_MATE_DEFEND_BONUS;

	//<https://www.chessprogramming.org/Killer_Heuristic>
	int16_t killers[128][2];
	static const int KILLER_BONUS;

};