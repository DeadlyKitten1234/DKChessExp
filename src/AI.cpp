#include "AI.h"

AI::AI() {
	pos = nullptr;
	bestMove = nullMove;
	evalGuess = new int[256]();
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
}
