#pragma once
#include <iostream>
#include <stdlib.h>
#include <algorithm>
#include "Position.h"
#include "Piece.h"
#include "Move.h"
using std::pair;
using std::abs;
using std::max;

extern uint64_t tileAttacked;
extern uint64_t tileDoubleAttacked;
extern uint64_t tileBlocksCheck;
extern int8_t tilePinnedDir[64];
extern int8_t nullPinnedDir;

extern inline bool tileAttackedBySlidingPiece(const Position& pos, int8_t tile, int8_t ignoreStTile = -1, int8_t addEndTile = -1, int8_t ignoreEnPassantCaptureTile = -1);

extern inline int16_t generateKingMoves(const Position& pos, int16_t* out, int16_t curMoveCnt, bool inCheck);

extern inline int16_t extractPawnMovesFromBitmask(int16_t* out, int16_t moveStIdx, uint64_t bitmask, int8_t enPassantTile, int8_t dirIdx, bool doubleMove, bool blackToMove, bool inCheck);
extern inline int16_t generatePawnMoves(const Position& pos, int16_t* out, int16_t moveStIdx, bool inCheck);

extern inline int16_t generateSlidingPieceMoves(const Position& pos, int16_t* out, int16_t curMoveCnt, Piece& piece, bool inCheck);
extern inline int16_t generateKnightMoves(const Position& pos, int16_t* out, int16_t curMoveCnt, Piece& knight, bool inCheck);

extern inline void addAttack(uint64_t bitmask);
extern inline void generateBishopAttackTiles(Piece* bishop, Piece* friendlyKing, uint64_t allPiecesBitboard);
extern inline void generateRookAttackTiles(Piece* rook, Piece* friendlyKing, uint64_t allPiecesBitboard);

extern inline void getAttackedTiles(const Position& pos);
extern inline int16_t generateMoves(const Position& pos, int16_t* out, int16_t outStIdx);

