#include "AI.h"

AI::AI() { pos = nullptr; }
AI::~AI() {
	if (pos != nullptr) {
		delete pos;
	}
}

void AI::initPos(Position* pos_) {
	pos = pos_;
}
