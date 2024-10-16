#pragma once
#include <cinttypes>
using std::int8_t;
using std::int16_t;

enum PieceType : int8_t {
	KING = 0,
	QUEEN = 1,
	BISHOP = 2,
	KNIGHT = 3,
	ROOK = 4,
	PAWN = 5,
	UNDEF = 6
};

class Position;

class Piece {
public:
	Piece();
	Piece(const int8_t x_, const int8_t y_, const PieceType type_, const bool black_);
	~Piece();

	void setPos(int8_t pos_);

	int8_t pos;		//readOnly
	int8_t x;		//readOnly
	int8_t y;		//readOnly
	int8_t diagMain;//readOnly
	int8_t diagScnd;//readOnly
	PieceType type;
	bool black;

	friend Position;
private:
	//This is index in Position::m_whitePiece or Position::m_blackPiece
	//Used for quickly swapping pieces when capturing
	//Variable should be managed by position
	int8_t idx;
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
	if (piece == PieceType::PAWN) { return 'p'; }
	return PieceType::UNDEF;
}