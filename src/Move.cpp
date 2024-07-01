#include "Move.h"
#include "PrecomputedData.h"

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