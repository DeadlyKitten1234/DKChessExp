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

	inline void orderMoves(int16_t startIdx, int16_t endIdx, int16_t* indices, int16_t ttBestMove = nullMove);
	int* evalGuess;//Used for ordering moves
	int historyHeuristic[2][64][64];

	int16_t bestMove;

	//Warning: pos is referece => changes made by the AI will impact the original position
	Position* pos;
};

inline int16_t AI::iterativeDeepening(int8_t depth) {
	for (int8_t i = 0; i < 64; i++) {
		for (int8_t j = 0; j < 64; j++) {
			historyHeuristic[0][i][j] = 0;
			historyHeuristic[1][i][j] = 0;
		}
	}
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
	const int16_t movesCnt = pos->m_legalMovesCnt;
	const int16_t* moves = pos->m_legalMoves + pos->m_legalMovesStartIdx;
	if (movesCnt == 0) {
		if (pos->friendlyInCheck) {
			return -pieceValue[KING];
		}
		return 0;
	}
	int16_t moveIndices[256];

	orderMoves(pos->m_legalMovesStartIdx, pos->m_legalMovesStartIdx + movesCnt, moveIndices, (foundTTEntry ? ttEntryRes->bestMove : nullMove));

	bool failLow = 1;//Fail low means no move gives a score > alpha; Starts with 1 and is set to 0 if a score > alpha is achieved
	const int8_t bitmaskCastling = pos->m_bitmaskCastling;//Used for undo-ing moves
	const int8_t possibleEnPassant = pos->m_possibleEnPassant;//Used for undo-ing moves
	for (int16_t i = 0; i < movesCnt; i++) {
		const int16_t curMove = moves[moveIndices[i]];
		const int8_t capturedPieceIdx = pos->makeMove(curMove);//int8_t declared is used to undo the move
		pos->m_legalMovesStartIdx += movesCnt;
		int16_t res = -search<0>(depth - 1, -beta, -alpha);
		pos->m_legalMovesStartIdx -= movesCnt;
		pos->undoMove(curMove, capturedPieceIdx, bitmaskCastling, possibleEnPassant);
		//Temporary
		//if (root) {
		//	std::cout << "Evaluating ";
		//	printName(curMove);
		//	std::cout << ": " << res << "; Alpha: " << alpha << "; Beta: " << beta << '\n';
		//}

		//If mate ? decrease value with depth
		if (res > pieceValue[KING] - 100) {
			res--;
		}
		//Alpha-beta cutoff
		if (res >= beta) {
			ttEntryRes->init(pos->zHash, curMove, res, depth, tt.gen, BoundType::LowBound);
			if constexpr (root) {
				pos->m_legalMovesCnt = movesCnt;
				bestMove = curMove;
			}
			historyHeuristic[pos->m_blackToMove][getStartPos(curMove)][getEndPos(curMove)] += depth * depth * depth;
			return beta;
		}
		//Best move
		if (res > alpha) {
			if constexpr (root) {
				bestMove = curMove;
			}
			curBestMove = curMove;
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
	int16_t staticEval = pos->evaluate();
	//When going to cut, don't tt.find, because its expensive
	if (staticEval >= beta) {
		return beta;
	}
	//Check tt
	bool foundTTEntry = 0;
	TTEntry* ttEntryRes = tt.find(pos->zHash, foundTTEntry);
	//Anyting written in tt will already contain all captures, so don't check depth
	if (foundTTEntry) {
		ttEntryRes->setGen(tt.gen);
		//If lower bound
		if (ttEntryRes->boundType() == BoundType::LowBound) {
			staticEval = ttEntryRes->eval;
		}
		//If exact bound => renew gen and return eval
		if (ttEntryRes->boundType() == BoundType::Exact) {
			return ttEntryRes->eval;
		}
	}
	bool failLow = 1;//Fail low means no move gives a score > alpha; Starts with 1 and is set to 0 if a score > alpha is achieved
	if (staticEval >= beta) {
		return beta;
	}
	if (staticEval > alpha) {
		alpha = staticEval;
		failLow = 0;
	}

	pos->updateLegalMoves<1>();
	const int16_t movesCnt = pos->m_legalMovesCnt;
	int16_t moveIndices[256];

	orderMoves(pos->m_legalMovesStartIdx, pos->m_legalMovesStartIdx + movesCnt, moveIndices, (foundTTEntry ? ttEntryRes->bestMove : nullMove));

	int16_t curBestMove = nullMove;
	const int16_t* moves = pos->m_legalMoves + pos->m_legalMovesStartIdx;
	const int8_t bitmaskCastling = pos->m_bitmaskCastling;//Used for undo-ing moves
	const int8_t possibleEnPassant = pos->m_possibleEnPassant;//Used for undo-ing moves
	for (int16_t i = 0; i < movesCnt; i++) {
		const int16_t curMove = moves[moveIndices[i]];
		int8_t capturedPieceIdx = pos->makeMove(curMove);//int8_t declared is used to undo the move
		pos->m_legalMovesStartIdx += movesCnt;
		int16_t res = -searchOnlyCaptures(-beta, -alpha);
		pos->m_legalMovesStartIdx -= movesCnt;
		pos->undoMove(curMove, capturedPieceIdx, bitmaskCastling, possibleEnPassant);
		if (res >= beta) {
			ttEntryRes->init(pos->zHash, curMove, res, 0, tt.gen, BoundType::LowBound);
			return beta;
		}
		if (res > alpha) {
			alpha = res;
			failLow = 0;
			curBestMove = curMove;
		}
	}
	ttEntryRes->init(pos->zHash, curBestMove, alpha, 0, tt.gen, (failLow ? BoundType::HighBound : BoundType::LowBound));
	return alpha;
}

inline void AI::orderMoves(int16_t startIdx, int16_t endIdx, int16_t* indices, int16_t ttBestMove) {
	//Note: here we guess based on the player to move
	//no need to complicate things with if (black) {} everywhere
	const uint64_t allPcs = pos->m_whiteAllPiecesBitboard | pos->m_blackAllPiecesBitboard;
	const int8_t enemyKingPos = (pos->m_blackToMove ? pos->m_whitePiece[0]->pos : pos->m_blackPiece[0]->pos);
	uint64_t checksToKing[6] = {
		0,//King can't check another king
		0,//Queen will be redundant if evaluated with rook and bishop, so after arr init do bitwise |
		attacks<BISHOP>(enemyKingPos, allPcs),
		attacks<KNIGHT>(enemyKingPos, allPcs),
		attacks<ROOK>(enemyKingPos, allPcs),
		0//maybe later add pawns
	};
	//Queen checks = rook checks | bishop checks
	checksToKing[QUEEN] = checksToKing[BISHOP] | checksToKing[ROOK];
	for (int16_t i = startIdx; i < endIdx; i++) {
		//Note: here we only guess how good a move is, so we can afford to make
		//guesses overinflated since the only thing that matters is their relative values
		int16_t curMove = pos->m_legalMoves[i];
		int8_t stPos = getStartPos(curMove), endPos = getEndPos(curMove);
		PieceType pt = pos->m_pieceOnTile[stPos]->type;
		int curGuess = 0;
		//Add bonuses; Have to have if(black) here, because template doesn't accept it as argument
		if (pos->m_blackToMove) {
			curGuess += 4 * (getSqBonus<1>(pt, endPos) - getSqBonus<1>(pt, stPos));
		} else {
			curGuess += 4 * (getSqBonus<0>(pt, endPos) - getSqBonus<0>(pt, stPos));
		}
		//Add history heuristic
		int flog2 = floorLog2(historyHeuristic[pos->m_blackToMove][stPos][endPos]);
		curGuess += flog2 * flog2 / 16;

		//Favor captures
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
			//Add pieceValue[piece to make move] / 16, because generally checks with
			//queens and rooks are better (and there is a higher chance for a fork)
			curGuess += 75 + (pieceValue[pt] >> 4);
		}

		//Favor move in tt
		if (curMove == ttBestMove) {
			curGuess = 0x7FFF;//<- just a sufficiently large number
		}

		//Write in guess array that will later be used to sort the moves
		evalGuess[i - startIdx] = curGuess;
		//In sortHelper store indices of moves
		indices[i - startIdx] = i - startIdx;
	}
	//Use indices to "link" evalGuess and pos->m_legalMove together because use std::sort
	std::sort(indices, indices + (endIdx - startIdx),
		[this](int16_t idx1, int16_t idx2) {
			return evalGuess[idx1] > evalGuess[idx2];
		}
	);
}