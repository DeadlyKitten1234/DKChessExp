#pragma once
#include "Position.h"
#include <algorithm>//for max and min
using std::max;
using std::min;
class AI {
public:
	AI();
	~AI();

	//Warning: pos is referece => changes made by the AI will impact the original position
	void initPos(Position* pos_);

	template<bool maximisingPlayer>
	int search(int8_t depth, int alpha, int beta);
	template<bool maximisingPlayer>
	int searchOnlyCaptures(int alpha, int beta);

	Position* pos;
	//static Transposition table

};


template<bool maximisingPlayer>
inline int AI::search(int8_t depth, int alpha, int beta) {
	if (depth == 0) {
		return searchOnlyCaptures<maximisingPlayer>(alpha, beta);
	}
	//here need to add if seen in tptable
	pos->updateLegalMoves<0>();
	int16_t movesCnt = pos->m_legalMovesCnt;
	const int16_t* moves = pos->m_legalMoves + pos->m_legalMovesStartIdx;
	if (movesCnt == 0) {
		if (pos->friendlyInCheck) {
			return (pos->m_blackToMove ? pieceValue[KING] : -pieceValue[KING]);
		}
		return 0;
	}

	const int8_t bitmaskCastling = pos->m_bitmaskCastling;//Used for undo-ing moves
	const int8_t possibleEnPassant = pos->m_possibleEnPassant;//Used for undo-ing moves
	for (int16_t i = 0; i < movesCnt; i++) {
		int8_t capturedPieceIdx = pos->makeMove(moves[i]);//int int8_t declared is used to undo the move
		pos->m_legalMovesStartIdx += movesCnt;
		int res = search<!maximisingPlayer>(depth - 1, alpha, beta);
		pos->undoMove(moves[i], capturedPieceIdx, bitmaskCastling, possibleEnPassant);
		pos->m_legalMovesStartIdx -= movesCnt;
		if constexpr (maximisingPlayer) {
			alpha = max(alpha, res);
		} else {
			beta = min(beta, res);
		}
		if (alpha >= beta) {
			break;
		}
	}
	if constexpr (maximisingPlayer) {
		return alpha;
	} else {
		return beta;
	}
}

template<bool maximisingPlayer>
inline int AI::searchOnlyCaptures(int alpha, int beta) {
	//Don't force player to capture if it is worse for him
	//(example: don't force Qxa1 if then there is bxa1)
	if constexpr (maximisingPlayer) {
		alpha = max(alpha, pos->evaluate());
	} else {
		beta = min(beta, pos->evaluate());
	}

	//here need to add if seen in tptable
	pos->updateLegalMoves<1>();
	int16_t movesCnt = pos->m_legalMovesCnt;
	const int16_t* moves = pos->m_legalMoves + pos->m_legalMovesStartIdx;
	const int8_t bitmaskCastling = pos->m_bitmaskCastling;//Used for undo-ing moves
	const int8_t possibleEnPassant = pos->m_possibleEnPassant;//Used for undo-ing moves
	for (int16_t i = 0; i < movesCnt; i++) {
		int8_t capturedPieceIdx = pos->makeMove(moves[i]);//int int8_t declared is used to undo the move
		pos->m_legalMovesStartIdx += movesCnt;
		int res = searchOnlyCaptures<!maximisingPlayer>(alpha, beta);
		pos->undoMove(moves[i], capturedPieceIdx, bitmaskCastling, possibleEnPassant);
		pos->m_legalMovesStartIdx -= movesCnt;
		if constexpr (maximisingPlayer) {
			alpha = max(alpha, res);
		} else {
			beta = min(beta, res);
		}
		if (alpha >= beta) {
			break;
		}
	}
	if constexpr (maximisingPlayer) {
		return alpha;
	} else {
		return beta;
	}
}

