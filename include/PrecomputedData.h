#pragma once
#include "misc.h"
#include "Piece.h"
#include <cinttypes>

extern const int8_t dirAdd[9];
extern const int8_t dirAddX[9];
extern const int8_t dirAddY[9];
extern const int8_t dirIdxFromCoordChange[3][3];//First y + 1, then x + 1
extern const int8_t knightMoveAdd[8];
extern const int8_t knightMoveAddX[8];
extern const int8_t knightMoveAddY[8];

//Misc
inline bool moveInbounds(int8_t x, int8_t y, int8_t dirIdx) {
	return (0 <= x + dirAddX[dirIdx] && x + dirAddX[dirIdx] <= 7
		&& 0 <= y + dirAddY[dirIdx] && y + dirAddY[dirIdx] <= 7);
}
inline bool knightMoveInbounds(int8_t x, int8_t y, int8_t knightMoveIdx) {
	return (0 <= x + knightMoveAddX[knightMoveIdx] && x + knightMoveAddX[knightMoveIdx] <= 7) &&
		(0 <= y + knightMoveAddY[knightMoveIdx] && y + knightMoveAddY[knightMoveIdx] <= 7);
}
//Bitboards and bitmasks with no magic
extern uint64_t knightMovesLookup[64];
extern uint64_t kingMovesLookup[64];
extern uint64_t rowBitmask[8];
extern uint64_t colBitmask[8];
extern uint64_t mainDiagBitmask[15];
extern uint64_t scndDiagBitmask[15];

extern uint64_t betweenBitboard[64][64];

extern uint64_t castlingImportantSquares[4];//First white; First kingside

//Magic bitboards
extern uint64_t rookMovesLookup[64][4096];
extern uint64_t bishopMovesLookup[64][4096];
//sz = 256, because i couldn't find numbers that fit in 8, 16, 32, 64, 128
extern uint8_t kingMovesCntLookup[64][256];//Also a magic hash lookup table; 
extern uint64_t rookMagic[64];
extern uint64_t bishopMagic[64];
extern uint64_t kingMagic[64];
extern int8_t rookRelevantSqCnt[64];
extern int8_t bishopRelevantSqCnt[64];
extern uint64_t rookRelevantSq[64];
extern uint64_t bishopRelevantSq[64];

extern inline void initData();

extern void populateMovementBitmasks();
extern void populateBetweemBitboards();
extern void populateMovesLookup();
extern void populateGlobalDirectionBitmasks();//Populates rowBitmask[8], colBitmask[8], mainDiagBitmask[15], scndDiagBitmask[15]
extern void populateKingMovesCnt();
extern uint64_t calcRookAtt(const int8_t stSquare, const uint64_t blockers);
extern uint64_t calcBishopAtt(const int8_t stSquare, const uint64_t blockers);
extern uint64_t shiftIndexToABlockersBitmask(const int idx, const uint64_t relevantSq);

extern inline int getMagicIdx(const uint64_t bitmask, const int sq, const bool bishop);

template<PieceType type>
inline uint64_t attacks(const int8_t sq, const uint64_t blockers) {
	if constexpr (type == KNIGHT) {
		return knightMovesLookup[sq];
	}
	if constexpr (type == KING) {
		return kingMovesLookup[sq];
	}
	if constexpr (type == BISHOP) {
		return bishopMovesLookup[sq][getMagicIdx(blockers & bishopRelevantSq[sq], sq, 1)];
	}
	if constexpr (type == ROOK) {
		return rookMovesLookup[sq][getMagicIdx(blockers & rookRelevantSq[sq], sq, 0)];
	}
	if constexpr (type == QUEEN) {
		return bishopMovesLookup[sq][getMagicIdx(blockers & bishopRelevantSq[sq], sq, 1)] |
			   rookMovesLookup[sq][getMagicIdx(blockers & rookRelevantSq[sq], sq, 0)];
	}
	return 0;
}
template<bool blackToMove>
inline uint64_t pawnAttacks(const int8_t sq) {
	if constexpr (blackToMove) {
		return (getX(sq) != 0 ? 1 << (sq - 8 - 1) : 0) | (getX(sq) != 7 ? 1 << (sq - 8 + 1) : 0);
	} else {
		return (getX(sq) != 0 ? 1 << (sq + 8 - 1) : 0) | (getX(sq) != 7 ? 1 << (sq + 8 + 1) : 0);
	}
}

inline int8_t kingMovesCnt(const int8_t pos, const uint64_t friendlyBB) {
	return kingMovesCntLookup[pos][((friendlyBB & kingMovesLookup[pos]) * kingMagic[pos]) >> 56];
}