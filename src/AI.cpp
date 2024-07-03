#include "AI.h"
#include <algorithm>//for max and min
using std::max;
using std::min;

AI::AI() { pos = nullptr; }
AI::~AI() {
	if (pos != nullptr) {
		delete pos;
	}
}

void AI::initPos(Position* pos_) {
	pos = pos_;
}