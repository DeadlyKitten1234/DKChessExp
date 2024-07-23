#include "Piece.h"

Piece::Piece() {
	pos = -1;
	x = -1;
	y = -1;
	diagMain = -1;
	diagScnd = -1;
	idx = -1;
	type = PieceType::UNDEF;
	black = 0;
}

Piece::Piece(const int8_t x_, const int8_t y_, const PieceType type_, const bool black_) {
	x = x_;
	y = y_;
	pos = (y << 3) + x;
	diagMain = x - y + 7;
	diagScnd = x + y;
	type = type_;
	black = black_;
}

Piece::~Piece() {}

void Piece::setPos(int8_t pos_) {
	pos = pos_;
	x = pos & 7;
	y = (pos & 56) >> 3;
	diagMain = x - y + 7;
	diagScnd = x + y;
}
