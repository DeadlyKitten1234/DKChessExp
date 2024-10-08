#include <iostream>
#include "Move.h"
#include "misc.h"
#include "PrecomputedData.h"

inline int16_t createMove(const int8_t stPos, const int8_t endPos, const bool givesCheck, const PieceType promotionPiece) {
	return (givesCheck << 15) | (int8_t(promotionPiece) << 12) | (stPos << 6) | endPos;
}

inline int8_t getStartPos(const int16_t move) {
	return ((move & 4032) >> 6);//4032 = 111111000000
}
inline int8_t getEndPos(const int16_t move) {
	return (move & 63);//63 = 111111
}
inline PieceType getPromotionPiece(const int16_t move) {
	return PieceType((move & 28672) >> 12);//28672 = 111000000000000
}

inline bool givesCheck(const int16_t move) {
	return move & (1 << 15);
}
inline bool isPromotion(const int16_t move) {
	return getPromotionPiece(move) != UNDEF;
}

void printName(const int16_t move) {
	std::cout << char(getX(getStartPos(move)) + 'a') << char(getY(getStartPos(move)) + '1') <<
				 char(getX(getEndPos(move)) + 'a')   << char(getY(getEndPos(move)) + '1');
	if (getPromotionPiece(move) != PieceType::UNDEF) {
		std::cout << getCharFromType(getPromotionPiece(move));
	}
}

int16_t getMoveFromText(const std::string move) {
	return createMove(move[0] - 'a' + (move[1] - '1') * 8, move[2] - 'a' + (move[3] - '1') * 8, (move.size() > 4 ? getTypeFromChar(move[4])  : UNDEF));
}
