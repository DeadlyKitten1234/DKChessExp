#include "Move.h"
#include "PrecomputedData.h"

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

inline int16_t createMove(const int8_t stPos, const int8_t endPos, const PieceType promotionPiece) {
	return (int8_t(promotionPiece) << 12) + (stPos << 6) + endPos;
}

inline int8_t getStartPos(int16_t move) {
	return ((move & 4032) >> 6);//4032 = 111111000000
}
inline int8_t getEndPos(int16_t move) {
	return (move & 63);//63 = 111111
}
inline PieceType getPromotionPiece(int16_t move) {
	return PieceType((move & 28672) >> 12);//28672 = 111000000000000
}

inline char* getTileName(int8_t tile) {
	char* ans = new char[2]();
	ans[0] = getX(tile) + 'a';
	ans[1] = getY(tile) + '1';
	return ans;
}