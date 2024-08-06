#include "AI.h"
const int AI::MAX_HISTORY = 625;
const int AI::COUNTER_MOVE_BONUS = 300;


AI::AI() {
	pos = nullptr;
	bestMove = nullMove;
	evalGuess = new int[256]();
	movesHistory.init(256);
}

AI::~AI() {
	if (pos != nullptr) {
		delete pos;
	}
	if (evalGuess != nullptr) {
		delete[] evalGuess;
	}
}

void AI::initPos(Position* pos_) {
	pos = pos_;
	resetHistory();
}

void AI::resetHistory() {
	//Reset hh and cmh
	for (int8_t i = 0; i < 6; i++) {
		for (int8_t j = 0; j < 64; j++) {
			historyHeuristic[0][i][j] = 0;
			historyHeuristic[1][i][j] = 0;
			counterMove[0][i][j] = nullMove;
			counterMove[1][i][j] = nullMove;
		}
	}
	movesHistory.clear();
}

void AI::updateHistoryNewSearch() {
	for (int8_t i = 0; i < 6; i++) {
		for (int8_t j = 0; j < 64; j++) {
			//Don't fully reset hh, but reduce it
			historyHeuristic[0][i][j] /= 3;
			historyHeuristic[1][i][j] /= 3;
			//Dont reset counter moves, because good chance that they will remain good
		}
	}
	movesHistory.clear();
}
