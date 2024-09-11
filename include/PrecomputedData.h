#pragma once
#include "misc.h"
#include "Piece.h"
#include <algorithm>
#include <cinttypes>
using std::clamp;

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
//[0] - white kingside
//[1] - white queenside
//[2] - black kingside
//[3] - black queenside
extern uint64_t castlingImportantSquares[4];

//Static exchange evaluation lookup table
//Note: here we FORCE opponent to capture, because we can just min(capture, noCapture)
//Parameters:
//    . 1st - kingsNum+1
//    . 2nd - pawnsNum+2
//    . 3rd - minorPcsNum+3
//    . 4th - rooksNum+2
//    . 5th - queensNum+1
//    . Opponent pieces are considered negative
extern int16_t seeLookup[3][5][7][5][3];

inline int16_t getSee(const int8_t kings, const int8_t queens, const int8_t bishops, const int8_t knights, const int8_t rooks, const int8_t pawns) {
	return seeLookup[clamp(kings + 1, 0, 2)][clamp(pawns + 2, 0, 4)]
					[clamp(bishops + knights + 3, 0, 6)][clamp(rooks + 2, 0, 4)]
					[clamp(queens + 1, 0, 2)];
}
inline int16_t getSee(const int8_t* pcsCnt) {
	return getSee(pcsCnt[KING], pcsCnt[QUEEN], pcsCnt[BISHOP], pcsCnt[KNIGHT], pcsCnt[ROOK], pcsCnt[PAWN]);
}
inline int16_t getSee(const int8_t* pcsCntFriend, const int8_t* pcsCntEnemy) {
	return getSee(	pcsCntFriend[KING] - pcsCntEnemy[KING],		pcsCntFriend[QUEEN] - pcsCntEnemy[QUEEN],
					pcsCntFriend[BISHOP] - pcsCntEnemy[BISHOP], pcsCntFriend[KNIGHT] - pcsCntEnemy[KNIGHT],
					pcsCntFriend[ROOK] - pcsCntEnemy[ROOK],		pcsCntFriend[PAWN] - pcsCntEnemy[PAWN]);
}

//Magic bitboards
extern uint64_t rookMovesLookup[64][4096];
extern uint64_t bishopMovesLookup[64][4096];
//Magic hash lookup table used for determining whether to check 
//if each king move is legal or to calculate enemy attacks
//sz = 256, because i couldn't find numbers that fit in 8, 16, 32, 64, 128
extern uint8_t kingMovesCntLookup[64][256];
extern const uint64_t rookMagic[64];
extern const uint64_t bishopMagic[64];
extern const uint64_t kingMagic[64];
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
extern void populateSeeLookup();
extern uint64_t calcRookAtt(const int8_t stSquare, const uint64_t blockers);
extern uint64_t calcBishopAtt(const int8_t stSquare, const uint64_t blockers);
extern uint64_t shiftIndexToABlockersBitmask(const int idx, const uint64_t relevantSq);

extern inline int getMagicIdx(const uint64_t bitmask, const int sq, const bool bishop);

template<PieceType type>
inline void prefetchAttacks(const int8_t sq, const uint64_t blockers) {
	if constexpr (type == KNIGHT) {
		prefetch((const char*)&knightMovesLookup[sq]);
	}
	if constexpr (type == KING) {
		prefetch((const char*)&kingMovesLookup[sq]);
	}
	if constexpr (type == BISHOP) {
		prefetch((const char*)bishopMovesLookup[sq][getMagicIdx(blockers & bishopRelevantSq[sq], sq, 1)]);
	}
	if constexpr (type == ROOK) {
		prefetch((const char*)rookMovesLookup[sq][getMagicIdx(blockers & rookRelevantSq[sq], sq, 0)]);
	}
	if constexpr (type == QUEEN) {
		prefetch((const char*)bishopMovesLookup[sq][getMagicIdx(blockers & bishopRelevantSq[sq], sq, 1)]);
		prefetch((const char*)rookMovesLookup[sq][getMagicIdx(blockers & rookRelevantSq[sq], sq, 0)]);
	}
}

template<PieceType type>
inline uint64_t attacks(const int8_t sq, const uint64_t blockers = 0) {
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
	if (getY(sq) == 0 && blackToMove) { return 0; }
	if (getY(sq) == 7 && !blackToMove) { return 0; }
	if constexpr (blackToMove) {
		return (getX(sq) != 0 ? 1ULL << (sq - 8 - 1) : 0) | (getX(sq) != 7 ? 1ULL << (sq - 8 + 1) : 0);
	} else {
		return (getX(sq) != 0 ? 1ULL << (sq + 8 - 1) : 0) | (getX(sq) != 7 ? 1ULL << (sq + 8 + 1) : 0);
	}
}

inline uint64_t pawnAttacks(const bool black, const int8_t sq) {
	return (black ? pawnAttacks<1>(sq) : pawnAttacks<0>(sq));
}

inline int8_t kingMovesCnt(const int8_t pos, const uint64_t friendlyBB) {
	return kingMovesCntLookup[pos][((friendlyBB & kingMovesLookup[pos]) * kingMagic[pos]) >> 56];
}