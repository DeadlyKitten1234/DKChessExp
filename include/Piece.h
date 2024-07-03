#pragma once
#include <cinttypes>
using std::int8_t;
using std::int16_t;

enum PieceType {
	KING = 0,
	QUEEN = 1,
	BISHOP = 2,
	KNIGHT = 3,
	ROOK = 4,
	PAWN = 5,
	UNDEF = 6
};

const int pieceValue[6] = { /*King value: INF = 1000000*/1000000, 1220, 400, 375, 610, 100 };

class Piece {
public:
	Piece();
	Piece(const int8_t x_, const int8_t y_, const PieceType type_, const bool black_);
	~Piece();

	void setPos(int8_t pos_);
	inline bool getMovesInDir(int8_t dirIdx) {
		return (!(dirIdx & 1) && (type == PieceType::QUEEN || type == PieceType::BISHOP)) ||
			((dirIdx & 1) && (type == PieceType::QUEEN || type == PieceType::ROOK));
	}

	int8_t pos;		//readOnly
	int8_t x;		//readOnly
	int8_t y;		//readOnly
	int8_t diagMain;//readOnly
	int8_t diagScnd;//readOnly
	PieceType type;	//readOnly
	bool black;		//readOnly
};

inline PieceType getTypeFromChar(char piece) {
	if (piece <= 'Z') { piece += 'a' - 'A'; }
	if (piece == 'k') { return PieceType::KING; }
	if (piece == 'q') { return PieceType::QUEEN; }
	if (piece == 'b') { return PieceType::BISHOP; }
	if (piece == 'n') { return PieceType::KNIGHT; }
	if (piece == 'r') { return PieceType::ROOK; }
	if (piece == 'p') { return PieceType::PAWN; }
	//throw string("Undefined piece " + piece);
	return PieceType::UNDEF;
}

inline char getCharFromType(PieceType piece) {
	if (piece == PieceType::KING) { return 'k'; }
	if (piece == PieceType::QUEEN) { return 'q'; }
	if (piece == PieceType::ROOK) { return 'r'; }
	if (piece == PieceType::KNIGHT) { return 'n'; }
	if (piece == PieceType::BISHOP) { return 'b'; }
	if (piece == PieceType::PAWN) { return ' '; }
	return PieceType::UNDEF;
}