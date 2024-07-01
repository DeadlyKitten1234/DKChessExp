#pragma once
#include <cinttypes>

extern int8_t numberOfSquaresToEdge[9][8][8];
extern const int8_t dirAdd[9];
extern const int8_t dirAddX[9];
extern const int8_t dirAddY[9];
extern const int8_t dirIdxFromCoordChange[3][3];//First y + 1, then x + 1
extern const int8_t knightMoveAdd[8];
extern const int8_t knightMoveAddX[8];
extern const int8_t knightMoveAddY[8];

inline int8_t getDirIdx(int8_t xChange, int8_t yChange) {
	return dirIdxFromCoordChange[yChange + 1][xChange + 1];
}
inline bool moveInbounds(int8_t x, int8_t y, int8_t dirIdx) {
	return (0 <= x + dirAddX[dirIdx] && x + dirAddX[dirIdx] <= 7
		&& 0 <= y + dirAddY[dirIdx] && y + dirAddY[dirIdx] <= 7);
}
inline bool knightMoveInbounds(int8_t x, int8_t y, int8_t knightMoveIdx) {
	return (0 <= x + knightMoveAddX[knightMoveIdx] && x + knightMoveAddX[knightMoveIdx] <= 7) &&
		(0 <= y + knightMoveAddY[knightMoveIdx] && y + knightMoveAddY[knightMoveIdx] <= 7);
}

inline int8_t getX(const int8_t pos) {
	return (pos & 7);//7 = 111
};
inline int8_t getY(const int8_t pos) {
	return (pos & 56) >> 3;//56 = 111000
};
inline int8_t getDiagM(const int8_t pos) {
	return getX(pos) - getY(pos) + 7;
}
inline int8_t getDiagS(const int8_t pos) {
	return getX(pos) + getY(pos);
}
extern void populateNumberOfSquaresToEdge();

inline int8_t getLSBPos(const uint64_t bitmask) {
	unsigned long idx;
	_BitScanForward64(&idx, bitmask);
	return idx;
}

//Bitboards and bitmasks with no magic
extern uint64_t knightMovesLookup[64];
extern uint64_t kingMovesLookup[64];
extern uint64_t rowBitmask[8];
extern uint64_t colBitmask[8];
extern uint64_t mainDiagBitmask[15];
extern uint64_t scndDiagBitmask[15];

extern uint64_t castlingImportantSquares[4];//First white; first kingside

//Magic bitboards
extern uint64_t rookMovesLookup[64][4096];
extern uint64_t bishopMovesLookup[64][4096];
extern uint64_t rookMagic[64];
extern uint64_t bishopMagic[64];
extern int8_t rookRelevantSqCnt[64];
extern int8_t bishopRelevantSqCnt[64];
extern uint64_t rookRelevantSq[64];
extern uint64_t bishopRelevantSq[64];

extern inline void initData();

extern void populateMovementBitmasks();
extern void populateMovesLookup();
extern void populateGlobalDirectionBitmasks();//Populates rowBitmask[8], colBitmask[8], mainDiagBitmask[15], scndDiagBitmask[15]
extern uint64_t calcRookAtt(const int8_t stSquare, const uint64_t blockers);
extern uint64_t calcBishopAtt(const int8_t stSquare, const uint64_t blockers);
extern uint64_t shiftIndexToABlockersBitmask(const int idx, const uint64_t relevantSq);

extern inline int getMagicIdx(const uint64_t bitmask, const int sq, const bool bishop);