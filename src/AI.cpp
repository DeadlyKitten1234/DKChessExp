#include "AI.h"

AI::AI() {
	pos = nullptr;
	bestMove = nullMove;
	evalGuess = new int[256]();
	sortHelper = new int16_t[256]();
}

AI::~AI() {
	if (pos != nullptr) {
		delete pos;
	}
	if (evalGuess != nullptr) {
		delete[] evalGuess;
	}
	if (sortHelper != nullptr) {
		delete[] sortHelper;
	}
}

void AI::initPos(Position* pos_) {
	pos = pos_;
}
