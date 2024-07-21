#include "ZobristHashing.h"
#include "misc.h"
uint64_t hashNums[64][2][6];//[Square][Color][PieceType]
uint64_t hashNumsCastling[16];
uint64_t hashNumsEp[8];//En passant
uint64_t hashNumBlackToMove = 0;

void populateHashNums() {
	//Both seed and random number generation have been taken from stockfish
	RandNumGen rng(1070372);

	for (int color = 0; color < 2; color++) {
		//Use this order, because stockfish does
		for (int sq = 0; sq < 64; sq++) { hashNums[sq][color][PAWN] = rng.rand(); }
		for (int sq = 0; sq < 64; sq++) { hashNums[sq][color][KNIGHT] = rng.rand(); }
		for (int sq = 0; sq < 64; sq++) { hashNums[sq][color][BISHOP] = rng.rand(); }
		for (int sq = 0; sq < 64; sq++) { hashNums[sq][color][ROOK] = rng.rand(); }
		for (int sq = 0; sq < 64; sq++) { hashNums[sq][color][QUEEN] = rng.rand(); }
		for (int sq = 0; sq < 64; sq++) { hashNums[sq][color][KING] = rng.rand(); }
		
	}
	for (int i = 0; i < 8; i++) {
		hashNumsEp[i] = rng.rand();
	}
	for (int i = 0; i < 16; i++) {
		hashNumsCastling[i] = rng.rand();
	}
	hashNumBlackToMove = rng.rand();
}

uint64_t getPositionHash(const Position& pos) {
	uint64_t ans = 0;
	for (int8_t i = 0; i < pos.m_whiteTotalPiecesCnt; i++) {
		ans ^= hashNums[pos.m_whitePiece[i]->pos][0][pos.m_whitePiece[i]->type];
	}
	for (int8_t i = 0; i < pos.m_blackTotalPiecesCnt; i++) {
		ans ^= hashNums[pos.m_blackPiece[i]->pos][1][pos.m_blackPiece[i]->type];
	}
	ans ^= hashNumsCastling[pos.m_bitmaskCastling];
	if (pos.m_possibleEnPassant != -1) {
		ans ^= hashNumsEp[pos.m_possibleEnPassant & 0b111];
	}
	if (pos.m_blackToMove) {
		ans ^= hashNumBlackToMove;
	}
	return ans;
}
