#include "AI.h"
const int AI::MAX_PLY_MATE = 256;
const int AI::CHECK_BONUS = 75;
const int AI::MAX_HISTORY = 625;
const int AI::COUNTER_MOVE_BONUS = 300;
const int AI::KILLER_BONUS = 550;
const int AI::NULL_MOVE_DEFEND_BONUS = 75;


AI::AI() {
	pos = nullptr;
	bestMove = nullMove;
	evalGuess = new int[MAX_LEGAL_MOVES]();
	movesHistory.init(256);
	searchEndTime = 0;
	inNullMoveSearch = 0;
	inScout = 0;
}

AI::~AI() {
	//if (pos != nullptr) {
	//	delete pos;
	//}
	if (evalGuess != nullptr) {
		delete[] evalGuess;
	}
}

void AI::initPos(Position* pos_) {
	pos = pos_;
	resetHistory();
}

int16_t AI::startSearch(uint64_t timeToSearch) {
	searchEndTime = Clock::now() + timeToSearch;
	//Reset killers
	for (uint8_t i = 0; i < 128; i++) {
		killers[i][0] = killers[i + 1][0] = nullMove;
	}
	tt.newGen();
	return iterativeDeepening(127);
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
	inNullMoveSearch = 0;
	inScout = 0;
	for (uint8_t i = 0; i < 128; i++) {
		killers[i][0] = killers[i][1] = nullMove;
	}
}

void AI::updateHistoryNewSearch() {
	for (int8_t i = 0; i < 6; i++) {
		for (int8_t j = 0; j < 64; j++) {
			//Don't fully reset hh, but reduce it
			historyHeuristic[0][i][j] /= 2;
			historyHeuristic[1][i][j] /= 2;
			//Dont reset counter moves, because good chance that they will remain good
		}
	}
	movesHistory.clear();
	inScout = 0;
	inNullMoveSearch = 0;
}
