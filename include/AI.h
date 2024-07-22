#pragma once
#include "Position.h"
#include "TranspositionTable.h"
#include <iostream>
#include <algorithm>//for max and min
using std::max;
using std::min;
class AI {
public:
	AI();
	~AI();

	//Warning: pos is referece => changes made by the AI will impact the original position
	void initPos(Position* pos_);
	int16_t iterativeDeepening(int8_t depth);
	template<bool root = 1>
	int16_t search(int8_t depth, int16_t alpha, int16_t beta);
	int16_t searchOnlyCaptures(int16_t alpha, int16_t beta);

	inline void orderMoves(int16_t startIdx, int16_t endIdx, int16_t ttBestMove = nullMove);
	int* evalGuess;//Used for ordering moves
	int16_t* sortHelper;//Used for sorting moves based on guesses

	int16_t bestMove;

	//Warning: pos is referece => changes made by the AI will impact the original position
	Position* pos;
};

inline int16_t AI::iterativeDeepening(int8_t depth) {
	int16_t eval = 0;
	for (int8_t i = 2; i < depth; i++) {
		eval = search(depth, -pieceValue[KING] - 1, pieceValue[KING] + 1);
	}
	return eval;
}

template<bool root>
inline int16_t AI::search(int8_t depth, int16_t alpha, int16_t beta) {
	if (depth == 0) {
		return searchOnlyCaptures(alpha, beta);
	}
	
	int16_t curBestMove = nullMove;
	bool foundTTEntry = 0;
	TTEntry* ttEntryRes = tt.find(pos->zHash, foundTTEntry);
	if (foundTTEntry && ttEntryRes->depth >= depth) {
		//Renew gen (aka make it more valuable when deciding which entry to overwrite)
		ttEntryRes->setGen(tt.gen);
		//Lower bound
		if (ttEntryRes->boundType() == BoundType::LowBound) {
			if (ttEntryRes->eval > alpha || ttEntryRes->eval >= beta) {
				if constexpr (root) {
					bestMove = ttEntryRes->bestMove;
				}
				curBestMove = ttEntryRes->bestMove;
			}
			if (ttEntryRes->eval >= beta) {
				return beta;
			}
			alpha = max(alpha, ttEntryRes->eval);
		}

		//Higher bound
		if (ttEntryRes->boundType() == BoundType::HighBound) {

		}
		//If exact bound => renew gen and return eval
		if (ttEntryRes->boundType() == BoundType::Exact) {
			if constexpr (root) {
				bestMove = ttEntryRes->bestMove;
			}
			return ttEntryRes->eval;
		}
	}

	pos->updateLegalMoves<0>();
	int16_t movesCnt = pos->m_legalMovesCnt;
	const int16_t* moves = pos->m_legalMoves + pos->m_legalMovesStartIdx;
	if (movesCnt == 0) {
		if (pos->friendlyInCheck) {
			return -pieceValue[KING];
		}
		return 0;
	}
	orderMoves(pos->m_legalMovesStartIdx, pos->m_legalMovesStartIdx + movesCnt, (foundTTEntry ? ttEntryRes->bestMove : nullMove));

	bool failLow = 1;//Fail low means no move gives a score > alpha; Starts with 1 and is set to 0 if a score > alpha is acihieved
	const int8_t bitmaskCastling = pos->m_bitmaskCastling;//Used for undo-ing moves
	const int8_t possibleEnPassant = pos->m_possibleEnPassant;//Used for undo-ing moves
	for (int16_t i = 0; i < movesCnt; i++) {
		int8_t capturedPieceIdx = pos->makeMove(moves[i]);//int8_t declared is used to undo the move
		pos->m_legalMovesStartIdx += movesCnt;
		int16_t res = -search<0>(depth - 1, -beta, -alpha);
		pos->m_legalMovesStartIdx -= movesCnt;
		pos->undoMove(moves[i], capturedPieceIdx, bitmaskCastling, possibleEnPassant);
		//Temporary
		//if (root) {
		//	std::cout << "Evaluating ";
		//	printName(moves[i]);
		//	std::cout << ": " << res << "; Alpha: " << alpha << "; Beta: " << beta << '\n';
		//}

		//If mate ? decrease value with depth
		if (res > pieceValue[KING] - 100) {
			res--;
		}
		//Alpha-beta cutoff
		if (res >= beta) {
			ttEntryRes->init(pos->zHash, moves[i], res, depth, tt.gen, BoundType::LowBound);
			if constexpr (root) {
				pos->m_legalMovesCnt = movesCnt;
				bestMove = moves[i];
			}
			return beta;
		}
		//Best move
		if (res > alpha) {
			if constexpr (root) {
				bestMove = moves[i];
			}
			curBestMove = moves[i];
			alpha = res;
			failLow = 0;
		}
	}
	if constexpr (root) {
		pos->m_legalMovesCnt = movesCnt;
	}
	//No alpha-beta cutoff occured
	ttEntryRes->init(pos->zHash, curBestMove, alpha, depth, tt.gen, (failLow ? BoundType::HighBound : BoundType::Exact));
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
	//Only read from tt, because evaluating only captures
	bool foundTTEntry = 0;
	TTEntry* ttEntryRes = tt.find(pos->zHash, foundTTEntry);
	if (foundTTEntry && ttEntryRes->depth >= 2) {
		//If lower bound
		if (ttEntryRes->boundType() == BoundType::LowBound) {
			if (ttEntryRes->eval >= beta) {
				ttEntryRes->setGen(tt.gen);
				return beta;
			}
			alpha = max(alpha, ttEntryRes->eval);
		}
		//If exact bound => renew gen and return eval
		if (ttEntryRes->boundType() == BoundType::Exact) {
			ttEntryRes->setGen(tt.gen);
			return ttEntryRes->eval;
		}
	}

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
	uint64_t checksToKing[6] = {
		attacks<KING>	(enemyKingPos, allPcs),
		attacks<QUEEN>	(enemyKingPos, allPcs),
		attacks<BISHOP>	(enemyKingPos, allPcs),
		attacks<KNIGHT>	(enemyKingPos, allPcs),
		attacks<ROOK>	(enemyKingPos, allPcs),
		0/*maybe later add pawns*/
	};
	for (int16_t i = startIdx; i < endIdx; i++) {
		//Note: here we only guess how good a move is, so we can afford to make
		//guesses overinflated since the only thing that matters is their relative values
		int16_t curMove = pos->m_legalMoves[i];
		int8_t stPos = getStartPos(curMove), endPos = getEndPos(curMove);
		PieceType pt = pos->m_pieceOnTile[stPos]->type;
		int curGuess = 0;
		//If capturing guess = val[capturedPiece] - val[captuingPece]
		if (pos->m_pieceOnTile[getEndPos(curMove)] != nullptr) {
			//13 * capturedPieceVal to prioritise captures before non-captures and 
			//to incentivise capturing more valuable pieces; We chosse 13, because
			//13 is the lowest number above how much pawns a queen is worth
			curGuess += 13 * pieceValue[pos->m_pieceOnTile[endPos]->type] - pieceValue[pt];
		}
		//If tile attacked by opponent pawn
		if ((1ULL << endPos) & pos->enemyPawnAttacksBitmask) {
			//mult by 4, because why not?
			curGuess -= 4 * pieceValue[pt];
		}

		//Promoting is good
		if (getPromotionPiece(curMove) != UNDEF) {
			//Multiply by 14, because when capturing we multiply 
			//by 13 and promoting is more likely to be good (I think)
			curGuess += 14 * pieceValue[getPromotionPiece(curMove)];
		}
		//Favor checks slightly
		if (checksToKing[pt] & (1ULL << endPos)) {
			//Add pieceValue[piece to make move] / 32, because generally checks with
			//queens and rooks are better (and there is a higher chance for a fork)
			curGuess += 150 + (pieceValue[pt] >> 5);
		}

		//Favor move in tt
		if (curMove == ttBestMove) {
			curGuess = 0x7FFF;//<- just a sufficiently large number
		}

		//Write in guess array that will later be used to sort the moves
		evalGuess[i - startIdx] = curGuess;
		//In sortHelper store indices of moves
		sortHelper[i - startIdx] = i - startIdx;
	}
	//Instead of sorting moves based on guesses with lambda or predicate
	//sort indices instead and then use them to reconstruct moves
	std::sort(sortHelper, sortHelper + (endIdx - startIdx),
		[this] (int16_t idx1, int16_t idx2) {
			return evalGuess[idx1] > evalGuess[idx2];
		}
	);

	//Convert indices in sortHelper to corresponding moves
	for (int16_t i = startIdx; i < endIdx; i++) {
		sortHelper[i - startIdx] = pos->m_legalMoves[sortHelper[i - startIdx] + startIdx];
	}
	for (int16_t i = startIdx; i < endIdx; i++) {
		pos->m_legalMoves[i] = sortHelper[i - startIdx];
	}
}