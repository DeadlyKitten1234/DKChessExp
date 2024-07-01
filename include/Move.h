#pragma once
#include "Piece.h"
#include <cmath>
#include <cinttypes>

extern inline int16_t createMove(const int8_t stPos, const int8_t endPos, const PieceType promotionPiece = PieceType::UNDEF);

extern inline int8_t getStartPos(int16_t move);
extern inline int8_t getEndPos(int16_t move);
extern inline PieceType getPromotionPiece(int16_t move);

extern inline char* getTileName(int8_t tile);