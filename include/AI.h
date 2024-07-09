#pragma once
#include "Position.h"
#include "TranspositionTable.h"
#include <algorithm>//for max and min
using std::max;
using std::min;
class AI {
public:
	AI();
	~AI();

	//Warning: pos is referece => changes made by the AI will impact the original position
	void initPos(Position* pos_);

	template<bool root = 1>
	int16_t search(int8_t depth, int16_t alpha, int16_t beta);
	int16_t searchOnlyCaptures(int16_t alpha, int16_t beta);

	inline void orderMoves(int16_t startIdx, int16_t endIdx, int16_t ttBestMove = nullMove);

	int16_t bestMove;

	//Warning: pos is referece => changes made by the AI will impact the original position
	Position* pos;
};
#include <iostream>
template<bool root>
inline int16_t AI::search(int8_t depth, int16_t alpha, int16_t beta) {
	if (depth == 0) {
		return searchOnlyCaptures(alpha, beta);
	}
	
	bool foundTTEntry = 0;
	TTEntry* ttEntryRes = tt.find(pos->zHash, foundTTEntry);
	//if (foundTTEntry && ttEntryRes->depth >= depth) {
	//	//If lower bound
	//	if (ttEntryRes->boundType() == BoundType::LowBound) {
	//		alpha = max(alpha, ttEntryRes->eval);
	//		if (alpha >= beta) {
	//			ttEntryRes->setGen(tt.gen);
	//			return beta;
	//		}
	//	}
	//	//If exact bound => renew gen and return eval
	//	if (ttEntryRes->boundType() == BoundType::Exact) {
	//		ttEntryRes->setGen(tt.gen);
	//		if constexpr (root) {
	//			bestMove = ttEntryRes->bestMove;
	//		}
	//		return ttEntryRes->eval;
	//	}
	//}

	pos->updateLegalMoves<0>();
	int16_t movesCnt = pos->m_legalMovesCnt;
	const int16_t* moves = pos->m_legalMoves + pos->m_legalMovesStartIdx;
	if (movesCnt == 0) {
		if (pos->friendlyInCheck) {
			return -pieceValue[KING];
		}
		return 0;
	}
	orderMoves(pos->m_legalMovesStartIdx, pos->m_legalMovesStartIdx + movesCnt/*, (foundTTEntry ? ttEntryRes->bestMove : nullMove)*/);

	const int8_t bitmaskCastling = pos->m_bitmaskCastling;//Used for undo-ing moves
	const int8_t possibleEnPassant = pos->m_possibleEnPassant;//Used for undo-ing moves
	for (int16_t i = 0; i < movesCnt; i++) {
		int8_t capturedPieceIdx = pos->makeMove(moves[i]);//int8_t declared is used to undo the move
		pos->m_legalMovesStartIdx += movesCnt;
		int16_t res = -search<0>(depth - 1, -beta, -alpha);
		if (root) {//Temporary
			printName(moves[i]);
			std::cout << ": " << res << '\n';
		}
		pos->m_legalMovesStartIdx -= movesCnt;
		pos->undoMove(moves[i], capturedPieceIdx, bitmaskCastling, possibleEnPassant);
		if (res >= beta) {
			ttEntryRes->init(pos->zHash, moves[i], res, depth, tt.gen, BoundType::LowBound);
			if constexpr (root) {
				pos->m_legalMovesCnt = movesCnt;
				bestMove = moves[i];
			}
			return beta;
		}
		if (res > alpha) {
			if constexpr (root) {
				bestMove = moves[i];
			}
			alpha = res;
		}
	}
	if constexpr (root) {
		pos->m_legalMovesCnt = movesCnt;
	}
	//No alpha-beta cutoff occured
	ttEntryRes->init(pos->zHash, bestMove, alpha, depth, tt.gen, BoundType::Exact);
	return alpha;
}

inline int16_t AI::searchOnlyCaptures(int16_t alpha, int16_t beta) {
	//Don't force player to capture if it is worse for him
	//(example: don't force Qxa1 if then there is bxa1)
	int16_t eval = pos->evaluate();
	if (eval >= beta) {
		return beta;
	}
	alpha = max(alpha, eval);
	//here need to add if seen in tptable
	pos->updateLegalMoves<1>();
	int16_t movesCnt = pos->m_legalMovesCnt;
	orderMoves(pos->m_legalMovesStartIdx, pos->m_legalMovesStartIdx + movesCnt);

	const int16_t* moves = pos->m_legalMoves + pos->m_legalMovesStartIdx;
	const int8_t bitmaskCastling = pos->m_bitmaskCastling;//Used for undo-ing moves
	const int8_t possibleEnPassant = pos->m_possibleEnPassant;//Used for undo-ing moves
	for (int16_t i = 0; i < movesCnt; i++) {
		int8_t capturedPieceIdx = pos->makeMove(moves[i]);//int8_t declared is used to undo the move
		pos->m_legalMovesStartIdx += movesCnt;
		int16_t res = -searchOnlyCaptures(-beta, -alpha);
		pos->m_legalMovesStartIdx -= movesCnt;
		pos->undoMove(moves[i], capturedPieceIdx, bitmaskCastling, possibleEnPassant);
		if (res >= beta) {
			return beta;
		}
		alpha = max(alpha, res);
	}
	return alpha;
}

inline void AI::orderMoves(int16_t startIdx, int16_t endIdx, int16_t ttBestMove) {
	//Note: here we guess based on the player to move
	//no need to complicate things with if (black) {} everywhere
	const uint64_t allPcs = pos->m_whiteAllPiecesBitboard | pos->m_blackAllPiecesBitboard;
	const int8_t enemyKingPos = (pos->m_blackToMove ? pos->m_whitePiece[0]->pos : pos->m_blackPiece[0]->pos);
	for (int16_t i = startIdx; i < endIdx; i++) {
		int maxGuess = -pieceValue[KING], maxGuessIdx = -1;
		for (int16_t j = i; j < endIdx; j++) {
			int16_t curMove = pos->m_legalMoves[j];
			int8_t stPos = getStartPos(curMove), endPos = getEndPos(curMove);
			int curGuess = 0;
			//If capturing guess = val[capturedPiece] - val[captuingPece]
			if (pos->m_pieceOnTile[getEndPos(curMove)] != nullptr) {
				//13 * capturedPieceVal to prioritise captures before non-captures and 
				//to incentivise capturing more valuable pieces; We chosse 13, because
				//13 is the lowest number above how much pawns a queen is worth
				curGuess += 13 * pieceValue[pos->m_pieceOnTile[endPos]->type] - pieceValue[pos->m_pieceOnTile[stPos]->type];
			}
			//Promoting is good
			if (getPromotionPiece(curMove) != UNDEF) {
				curGuess += pieceValue[getPromotionPiece(curMove)];
			}
			//Favor checks slightly
			PieceType type = pos->m_pieceOnTile[stPos]->type;
			if (attacks(type, enemyKingPos, allPcs) & (1ULL << endPos)) {
				curGuess += 85;
			}

			//Favor move in tt
			if (curMove == ttBestMove) {
				curGuess = 0xEFFF;//<- just a sufficiently large number
			}

			if (curGuess > maxGuess) {
				maxGuess = curGuess;
				maxGuessIdx = j;
			}
		}
		int16_t swapTemp = pos->m_legalMoves[i];
		pos->m_legalMoves[i] = pos->m_legalMoves[maxGuessIdx];
		pos->m_legalMoves[maxGuessIdx] = swapTemp;
	}
}