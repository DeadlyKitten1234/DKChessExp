#pragma once
#include "Piece.h"
#include <cmath>
#include <cinttypes>
#include <string>

const int16_t nullMove = 0x6000;

extern inline int16_t createMove(const int8_t stPos, const int8_t endPos, const bool givesCheck, const PieceType promotionPiece = PieceType::UNDEF);

extern inline int8_t getStartPos(const int16_t move);
extern inline int8_t getEndPos(const int16_t move);
extern inline PieceType getPromotionPiece(const int16_t move);
extern inline bool givesCheck(const int16_t move);
extern inline bool isPromotion(const int16_t move);

extern void printName(const int16_t move);
extern int16_t getMoveFromText(const std::string move);