#pragma once
#include <iostream>
#include "Position.h"
#include "PrecomputedData.h"
#include "Piece.h"
#include "Move.h"
using std::abs;

extern uint64_t tileBlocksCheck;
extern uint64_t pinnedPossibleMoves[64];
extern uint64_t tilePinnedBitmask;
extern int8_t nullPinnedDir;
extern bool inCheck;
extern bool inDoubleCheck;

inline uint64_t attackersToSq(const Position& pos, int8_t sq, int8_t ignoreStTile = -1) {
	uint64_t allBB = (pos.m_whiteAllPiecesBitboard | pos.m_blackAllPiecesBitboard) & ~(1ULL << sq);
	if (ignoreStTile != 0) {
		allBB &= ~(1ULL << ignoreStTile);
	}
	const uint64_t* enemyBB = (pos.m_blackToMove ? pos.m_whiteBitboards : pos.m_blackBitboards);
	return	(knightMovesLookup[sq] & enemyBB[KNIGHT]) |
			(kingMovesLookup[sq] & enemyBB[KING]) |
			(shift((1ULL << sq) & ~colBitmask[0], (pos.m_blackToMove ? -8 : 8) - 1) & enemyBB[PAWN]) |
			(shift((1ULL << sq) & ~colBitmask[7], (pos.m_blackToMove ? -8 : 8) + 1) & enemyBB[PAWN]) |
			(attacks<BISHOP>(sq, allBB) & (enemyBB[BISHOP] | enemyBB[QUEEN])) |
			(attacks<ROOK>(sq, allBB) & (enemyBB[ROOK] | enemyBB[QUEEN]));
}

inline bool tileAttackedBySlidingPiece(const Position& pos, int8_t tile, int8_t ignoreStTile, int8_t addEndTile, int8_t ignoreEnPassantCaptureTile) {
	uint64_t blockers = (pos.m_whiteAllPiecesBitboard | pos.m_blackAllPiecesBitboard);
	blockers ^= (1ULL << ignoreStTile) | (1ULL << ignoreEnPassantCaptureTile);
	blockers |= 1ULL << addEndTile;
	uint64_t enemyDiagPcs = (pos.m_blackToMove ? pos.m_whiteBitboards[BISHOP] | pos.m_whiteBitboards[QUEEN] :
											 	 pos.m_blackBitboards[BISHOP] | pos.m_blackBitboards[QUEEN]);
	if (bishopMovesLookup[tile][getMagicIdx(blockers & bishopRelevantSq[tile], tile, 1)] & enemyDiagPcs) {
		return 1;
	}
	uint64_t enemyStraightPcs = (pos.m_blackToMove ? pos.m_whiteBitboards[ROOK] | pos.m_whiteBitboards[QUEEN] :
													 pos.m_blackBitboards[ROOK] | pos.m_blackBitboards[QUEEN]);
	if (rookMovesLookup[tile][getMagicIdx(blockers & rookRelevantSq[tile], tile, 0)] & enemyStraightPcs) {
		return 1;
	}
	return 0;
}

inline int16_t generateKingMoves(const Position& pos, int16_t* out, int16_t curMoveCnt) {
	int16_t ans = curMoveCnt;
	Piece* king = (pos.m_blackToMove ? pos.m_blackPiece[0] : pos.m_whitePiece[0]);
	//Lookup moves and remove ones that have friendly pieces on them
	uint64_t possibleMoves = kingMovesLookup[king->pos] & ~(pos.m_blackToMove ? pos.m_blackAllPiecesBitboard : pos.m_whiteAllPiecesBitboard);
	uint64_t allPiecesBitboard = pos.m_whiteAllPiecesBitboard | pos.m_blackAllPiecesBitboard;
	//Castling
	bool canCastleKingside = (pos.m_bitmaskCastling & (pos.m_blackToMove ? 2 : 8)) != 0;//blackToMove ? 0010 : 1000
	bool canCastleQueenside = (pos.m_bitmaskCastling & (pos.m_blackToMove ? 1 : 4)) != 0;//blackToMove ? 0001 : 0100
	if (canCastleKingside && !inCheck) {
		uint64_t importantSq = castlingImportantSquares[(pos.m_blackToMove ? 2 : 0)];
		//No pieces in the way
		if (!(importantSq & allPiecesBitboard)) {
			//Can't castle through check
			bool illegalCastling = 0;
			while (importantSq) {
				if (attackersToSq(pos, getLSBPos(importantSq), king->pos)) {
					illegalCastling = 1;
					break;
				}
				importantSq &= importantSq - 1;
			}
			if (!illegalCastling) {
				//Directly add move, because not worth |= 1 << newPos to just add it later
				out[ans++] = createMove(king->pos, king->pos + 2);
			}
		}
	}
	if (canCastleQueenside && !inCheck) {
		uint64_t importantSq = castlingImportantSquares[(pos.m_blackToMove ? 3 : 1)];
		//No pieces in the way
		if (!(importantSq & allPiecesBitboard)) {
			//Can't castle through check
			//pop lsb is here, b1 (or b7 for black) can be attacked and can still castle 
			importantSq &= importantSq - 1;
			bool illegalCastling = 0;
			while (importantSq) {
				if (attackersToSq(pos, getLSBPos(importantSq), king->pos)) {
					illegalCastling = 1;
					break;
				}
				importantSq &= importantSq - 1;
			}
			if (!illegalCastling) {
				//Directly add move, because not worth |= 1 << newPos to just add it later
				out[ans++] = createMove(king->pos, king->pos - 2);

			}
		}
	}
	//Extract moves from bitmask
	while (possibleMoves != 0) {
		int8_t curPos = getLSBPos(possibleMoves);
		if (!attackersToSq(pos, curPos, king->pos)) {
			out[ans++] = createMove(king->pos, curPos);
		}
		possibleMoves &= possibleMoves - 1;
	}
	return ans - curMoveCnt;
}

inline int16_t extractPawnMovesFromBitmask(int16_t* out, int16_t moveStIdx, uint64_t bitmask, int8_t dirIdx, bool doubleMove, bool blackToMove) {
	int16_t ans = moveStIdx;
	while (bitmask != 0) {
		int8_t pos = getLSBPos(bitmask);
		int8_t stPos = pos - dirAdd[dirIdx] - (doubleMove ? dirAdd[dirIdx] : 0);
		if ((tilePinnedBitmask & (1ULL << stPos)) == 0 || pinnedPossibleMoves[stPos] & (1ULL << pos)) {
			if ((blackToMove && pos < 8) || (!blackToMove && pos > 55)) {
				for (int8_t i = QUEEN; i <= ROOK; i++) {
					out[ans++] = createMove(stPos, pos, PieceType(i));
				}
			} else {
				out[ans++] = createMove(stPos, pos);
			}
		}
		bitmask &= bitmask - 1;
	}
	return ans - moveStIdx;
}

inline int16_t generatePawnMoves(const Position& pos, int16_t* out, int16_t moveStIdx) {
	int16_t ans = moveStIdx;
	const uint64_t friendlyPawnsBitboard = (pos.m_blackToMove ? pos.m_blackBitboards[PAWN] : pos.m_whiteBitboards[PAWN]);
	const uint64_t emptySq = ~(pos.m_whiteAllPiecesBitboard | pos.m_blackAllPiecesBitboard);
	uint64_t moveB1, moveB2;
	//Regular and double move
	if (pos.m_blackToMove) {
		moveB1 = (friendlyPawnsBitboard >> 8) & emptySq;
		moveB2 = (moveB1 >> 8) & emptySq & rowBitmask[4];
	} else {
		moveB1 = (friendlyPawnsBitboard << 8) & emptySq;
		moveB2 = (moveB1 << 8) & emptySq & rowBitmask[3];
	}
	if (inCheck) {
		moveB1 &= tileBlocksCheck;
		moveB2 &= tileBlocksCheck;
	}
	int8_t dirIdx = dirIdxFromCoordChange[(pos.m_blackToMove ? 0 : 2)][1];
	ans += extractPawnMovesFromBitmask(out, ans, moveB1, dirIdx, 0, pos.m_blackToMove);
	ans += extractPawnMovesFromBitmask(out, ans, moveB2, dirIdx, 1, pos.m_blackToMove);
	//Captures
	uint64_t epBitboard1 = 0;
	uint64_t epBitboard2 = 0;
	if (pos.m_possibleEnPassant != -1) {
		int8_t enPassantablePawnPos = pos.m_possibleEnPassant + (pos.m_blackToMove ? 8 : -8);
		int8_t pawnToEnPassantPos = enPassantablePawnPos + 1;
		int8_t friendlyKingPos = (pos.m_blackToMove ? pos.m_blackPiece[0]->pos : pos.m_whitePiece[0]->pos);
		//Check if after en passant pawn will reveal attack on king
		if (!tileAttackedBySlidingPiece(pos, friendlyKingPos, pawnToEnPassantPos, pos.m_possibleEnPassant, enPassantablePawnPos)) {
			epBitboard1 = (1ULL << pos.m_possibleEnPassant);
		}
		pawnToEnPassantPos -= 2;
		//Check if after en passant pawn will reveal attack on king
		if (!tileAttackedBySlidingPiece(pos, friendlyKingPos, pawnToEnPassantPos, pos.m_possibleEnPassant, enPassantablePawnPos)) {
			epBitboard2 = (1ULL << pos.m_possibleEnPassant);
		}
	}
	if (pos.m_blackToMove) {
		moveB1 = ((friendlyPawnsBitboard & ~colBitmask[0]) >> 9) & (pos.m_whiteAllPiecesBitboard | epBitboard1);
		moveB2 = ((friendlyPawnsBitboard & ~colBitmask[7]) >> 7) & (pos.m_whiteAllPiecesBitboard | epBitboard2);
	} else {
		moveB1 = ((friendlyPawnsBitboard & ~colBitmask[0]) << 7) & (pos.m_blackAllPiecesBitboard | epBitboard1);
		moveB2 = ((friendlyPawnsBitboard & ~colBitmask[7]) << 9) & (pos.m_blackAllPiecesBitboard | epBitboard2);
	}
	if (inCheck) {
		moveB1 &= tileBlocksCheck | epBitboard1;
		moveB2 &= tileBlocksCheck | epBitboard2;
	}
	dirIdx = dirIdxFromCoordChange[(pos.m_blackToMove ? 0 : 2)][0];
	ans += extractPawnMovesFromBitmask(out, ans, moveB1, dirIdx, 0, pos.m_blackToMove);
	dirIdx = dirIdxFromCoordChange[(pos.m_blackToMove ? 0 : 2)][2];
	ans += extractPawnMovesFromBitmask(out, ans, moveB2, dirIdx, 0, pos.m_blackToMove);
	return ans - moveStIdx;
}

template<PieceType type>
inline int16_t generatePieceMoves(const Position& pos, int16_t* out, int16_t moveStIdx) {
	int16_t ans = moveStIdx;
	uint64_t pcsBitboard = (pos.m_blackToMove ? pos.m_blackBitboards[type] : pos.m_whiteBitboards[type]);
	const uint64_t firendlyPcsBitboard = (pos.m_blackToMove ? pos.m_blackAllPiecesBitboard : pos.m_whiteAllPiecesBitboard);
	const uint64_t allPcs = pos.m_whiteAllPiecesBitboard | pos.m_blackAllPiecesBitboard;
	while (pcsBitboard != 0) {
		int8_t piecePos = getLSBPos(pcsBitboard);
		uint64_t movesBitboard = attacks<type>(piecePos, allPcs) & ~firendlyPcsBitboard;
		if (inCheck) {
			movesBitboard &= tileBlocksCheck;
		}
		if (tilePinnedBitmask & (1ULL << piecePos)) {
			movesBitboard &= pinnedPossibleMoves[piecePos];
		}
		while (movesBitboard != 0) {
			int8_t destTile = getLSBPos(movesBitboard);
			out[ans++] = createMove(piecePos, destTile);
			movesBitboard &= movesBitboard - 1;
		}

		pcsBitboard &= pcsBitboard - 1;
	}
	return ans - moveStIdx;
}

inline void getPinsAndChecks(const Position& pos) {
	inCheck = 0;
	inDoubleCheck = 0;
	tilePinnedBitmask = 0;
	tileBlocksCheck = 0;
	const uint64_t* enemyBB = (pos.m_blackToMove ? pos.m_whiteBitboards : pos.m_blackBitboards);
	const uint64_t allPcsBB = pos.m_whiteAllPiecesBitboard | pos.m_blackAllPiecesBitboard;
	const uint64_t kingBB = (pos.m_blackToMove ? pos.m_blackBitboards[KING] : pos.m_whiteBitboards[KING]);
	const int8_t kingPos = (pos.m_blackToMove ? pos.m_blackPiece[0]->pos : pos.m_whitePiece[0]->pos);
	if (knightMovesLookup[kingPos] & enemyBB[KNIGHT]) {
		tileBlocksCheck |= knightMovesLookup[kingPos] & enemyBB[KNIGHT];
		inDoubleCheck = inCheck;
		inCheck = 1;
	}
	//Here black ? 8 : -8, because pawns are the other color
	const uint64_t pawnLeft = shift(enemyBB[PAWN] & ~colBitmask[0], (pos.m_blackToMove ? 8 : -8) - 1);
	const uint64_t pawnRight = shift(enemyBB[PAWN] & ~colBitmask[7], (pos.m_blackToMove ? 8 : -8) + 1);
	if (pawnLeft & kingBB || pawnRight & kingBB) {
		inDoubleCheck = inCheck;
		inCheck = 1;
		tileBlocksCheck |= shift((kingBB & ~(colBitmask[0])), (pos.m_blackToMove ? -8 : 8) - 1) & enemyBB[PAWN];
		tileBlocksCheck |= shift((kingBB & ~(colBitmask[7])), (pos.m_blackToMove ? -8 : 8) + 1) & enemyBB[PAWN];
	}
	uint64_t slidingBB = ((enemyBB[BISHOP] | enemyBB[QUEEN]) & attacks<BISHOP>(kingPos, 0)) | ((enemyBB[ROOK] | enemyBB[QUEEN]) & attacks<ROOK>(kingPos, 0));
	while (slidingBB != 0) {
		int8_t piecePos = getLSBPos(slidingBB);
		uint64_t pcsBetween = betweenBitboard[kingPos][piecePos] & allPcsBB;
		//If in check
		if (pcsBetween == 0) {
			inDoubleCheck = inCheck;
			inCheck = 1;
			tileBlocksCheck |= betweenBitboard[kingPos][piecePos];
			tileBlocksCheck |= 1ULL << piecePos;
		} else {
			//If only one piece in between sliding piece and 
			if ((pcsBetween & pcsBetween - 1) == 0) {
				//Here we never check if piece is friendly, but doesn't matter, because we won't try move it anyway
				tilePinnedBitmask |= pcsBetween;
				pinnedPossibleMoves[getLSBPos(pcsBetween)] = betweenBitboard[kingPos][piecePos] | (1ULL << piecePos);
			}
		}
		slidingBB &= slidingBB - 1;
	}
}

//Returns size
inline int16_t generateMoves(const Position& pos, int16_t* out, int16_t outStIdx) {
	Piece** friendlyPiece = (pos.m_blackToMove ? pos.m_blackPiece : pos.m_whitePiece);
	getPinsAndChecks(pos);
	int16_t movesCnt = outStIdx;
	movesCnt += generateKingMoves(pos, out, movesCnt);
	if (inDoubleCheck) {
		return movesCnt - outStIdx;
	}
	//Pawns
	movesCnt += generatePawnMoves(pos, out, movesCnt);
	movesCnt += generatePieceMoves<KNIGHT>(pos, out, movesCnt);
	movesCnt += generatePieceMoves<QUEEN>(pos, out, movesCnt);
	movesCnt += generatePieceMoves<ROOK>(pos, out, movesCnt);
	movesCnt += generatePieceMoves<BISHOP>(pos, out, movesCnt);
	return movesCnt - outStIdx;
}
