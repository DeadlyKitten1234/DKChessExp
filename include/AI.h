#pragma once
#include "TranspositionTable.h"
#include "Position.h"
#include "Clock.h"
#include "misc.h"
#include <iostream>
#include <xmmintrin.h>//_mm_prefetch
#include <algorithm>
using std::max;
using std::min;

enum NodeType : int8_t {
	//PV (principal variation(i think)) nodes are the 
	//nodes that are hit when players do the best moves
	PV = 0,
	//NonPv nodes can be cut-nodes (where alpha beta cutoff occured)
	//or all-nodes (where no score improved alpha)
	NonPV = 1,
	Root = 2
};

class AI {
public:
	AI();
	~AI();

	void initPos(Position* pos_);
	int16_t startSearch(uint64_t timeToSearch);
	int16_t bestMove;

private:
	int64_t searchEndTime;

	//Warning: pos is referece => changes made by the AI will impact the original position
	Position* pos;

	int16_t iterativeDeepening(int8_t depth);
	template<NodeType nodeType>
	int16_t search(int8_t depth, int16_t alpha, int16_t beta);
	int16_t searchOnlyCaptures(int16_t alpha, int16_t beta);
	inline void orderMoves(int16_t moves[256], int16_t movesCnt, int16_t* indices, int16_t ttBestMove, int16_t bestMoveAfterNull);
	void resetHistory();
	void updateHistoryNewSearch();

	static const int MAX_PLY_MATE;
	//Used for ordering moves
	int* evalGuess;
	Stack<int16_t> movesHistory;
	static const int CHECK_BONUS;

	int inScout;

	//<https://www.chessprogramming.org/History_Heuristic>
	int historyHeuristic[2][6][64];
	static const int MAX_HISTORY;

	//<https://www.chessprogramming.org/Countermove_Heuristic>
	int16_t counterMove[2][6][64];
	static const int COUNTER_MOVE_BONUS;

	//<https://www.chessprogramming.org/Null_Move_Pruning>
	int inNullMoveSearch;
	static const int NULL_MOVE_DEFEND_BONUS;

	//<https://www.chessprogramming.org/Killer_Heuristic>
	int16_t killers[128][2];
	static const int KILLER_BONUS;

};
#include <iomanip>//setw() when outputing info
inline int16_t AI::iterativeDeepening(int8_t depth) {
	int16_t eval = 0;
	for (int8_t i = min<int8_t>(depth, 2); i <= depth; i++) {
		//Don't oversaturate history heuristic moves that are near the root
		updateHistoryNewSearch();
		int16_t curEval = search<Root>(i, -pieceValue[KING] - 1, pieceValue[KING] + 1);

		if (Clock::now() >= searchEndTime) {
			std::cout << "Search ended { Eval: " << eval << "; Best move: ";
			printName(bestMove);
			std::cout << "; }\n";
			return eval;
		}
		//std::setw(x) makes the next output have a min width of x (it just adds spaces)
		std::cout << "Depth " << std::setw(2) << (int)i << ": { Eval = " << std::setw(5) << curEval << "; Best move = ";
		printName(bestMove);
		std::cout << "; }; Time remaining: ";
		std::cout << std::max<int64_t>(0, searchEndTime - Clock::now()) << '\n';
		eval = curEval;
	}
	return eval;
}

/*
	Search scheme:
	if depth = 0 ? qsearch
	update legal moves
	razoring
	futility pruning
	null move pruning
	lookup in tt
	search moves {
		reverse futility pruning
		search children
		uncertainty cutoffs
	}
	write in tt
*/
template<NodeType nodeType>
inline int16_t AI::search(int8_t depth, int16_t alpha, int16_t beta) {
	if (Clock::now() >= searchEndTime) {
		return 0;
	}
	if (depth <= 0) {
		return searchOnlyCaptures(alpha, beta);
	}
	constexpr bool root = (nodeType == Root);
	constexpr bool pvNode = (nodeType != NonPV);

	const int8_t bitmaskCastling = pos->m_bitmaskCastling;//Used for undo-ing moves
	const int8_t possibleEnPassant = pos->m_possibleEnPassant;//Used for undo-ing moves

	int16_t eval = pos->evaluate();

	int16_t moves[256];
	pos->updateLegalMoves<0>(moves, true);
	int16_t movesCnt = pos->m_legalMovesCnt;
	if (movesCnt == 0) {
		if (pos->friendlyInCheck) {
			return -pieceValue[KING];
		}
		return 0;
	}
	const bool abCloseToMate =	abs((abs(alpha) - pieceValue[KING])) <= MAX_PLY_MATE ||
								abs((abs(beta) - pieceValue[KING])) <= MAX_PLY_MATE;
	//Razoring
	if (!abCloseToMate && depth == 3 && eval + pieceValue[PAWN] * 2  + pvNode * 35 <= alpha &&
		(pos->m_blackToMove ? pos->m_whiteTotalPiecesCnt : pos->m_blackTotalPiecesCnt) >= 3) {

		depth--;
	}

	//Futility pruning; https://www.chessprogramming.org/Futility_Pruning
	if (!pvNode && !pos->friendlyInCheck && depth <= 9 && !abCloseToMate) {
		if (eval - pieceValue[BISHOP] * depth / 2 >= beta) {
			return eval - pieceValue[BISHOP] * depth / 2;
		}
		if (eval + pieceValue[BISHOP] * depth / 2 <= alpha) {
			return alpha;
		}
	}

	//Null move; https://www.chessprogramming.org/Null_Move_Pruning
	int16_t bestMoveAfterNull = nullMove;
	const bool lastWasNull = (!movesHistory.empty() && movesHistory.top() == nullMove);
	if (!pvNode && !pos->friendlyInCheck && pos->hasNonPawnPiece(pos->m_blackToMove) && !abCloseToMate && eval >= beta) {
		//Don't chain a lot of null moves, but if last was null and in 
		//only one null move search, do null movee to detect zugzwang
		if (inNullMoveSearch == lastWasNull) {
			//Depth reduction formula taken from stockfish and changed a little
			int8_t depthReduction = std::clamp(int(eval - beta) / pieceValue[PAWN], 0, 5) + 4 + depth / 3;
			if (lastWasNull) {
				//If last was null check another null move to detect zugzwang without reducing depth
				depthReduction = 0;
			}
			//Make move
			pos->makeNullMove();
			movesHistory.push(nullMove);
			inNullMoveSearch++;
			//Search
			int16_t nullRes = -search<NonPV>(depth - depthReduction, -beta, -beta + 1);
			//Get best move after null move
			bool foundNullMoveTTE = 0;
			TTEntry* nullMoveTTE = tt.find(pos->zHash, foundNullMoveTTE);
			if (foundNullMoveTTE) {
				bestMoveAfterNull = nullMoveTTE->bestMove;
			}
			//Undo move
			inNullMoveSearch--;
			pos->undoNullMove(possibleEnPassant);
			movesHistory.pop();
			//Prune
			if (nullRes >= beta) {
				if (depth <= 8) {
					return nullRes;
				}
				//Don't do null moves in verification search
				inNullMoveSearch += 3;
				int16_t verification = search<NonPV>(depth - depthReduction, beta - 1, beta);
				inNullMoveSearch -= 3;
				if (verification >= beta) {
					return verification;
				}
			}
		}
	}

	//Fail low means no move gives a score > alpha
	//Starts with 1 and is set to 0 if a score > alpha is achieved
	bool failLow = 1;
	bool foundTTEntry = 0;
	TTEntry* ttEntryRes = tt.find(pos->zHash, foundTTEntry);
	int16_t bestValue = -pieceValue[KING];

	int16_t curBestMove = nullMove;
	int16_t ttBestMove = (foundTTEntry ? ttEntryRes->bestMove : nullMove);
	if (foundTTEntry && ttEntryRes->depth >= depth) {
		//Renew gen (make it more valuable when deciding which entry to overwrite)
		ttEntryRes->setGen(tt.gen);
		//Lower bound
		if (ttEntryRes->boundType() == BoundType::LowBound) {
			if (ttEntryRes->eval > alpha || ttEntryRes->eval >= beta) {
				if (root) {
					bestMove = ttEntryRes->bestMove;
				}
				curBestMove = ttEntryRes->bestMove;
				failLow = 0;
			}
			if (ttEntryRes->eval >= beta) {
				return ttEntryRes->eval;
			}
			alpha = max(alpha, ttEntryRes->eval);
			eval = max(eval, ttEntryRes->eval);
		}
		//Higher bound
		if (ttEntryRes->boundType() == BoundType::HighBound) {
			//We have guaranteed a better position
			if (ttEntryRes->eval <= alpha) {
				return alpha;
			}
		}
		//If exact bound => return eval
		if (ttEntryRes->boundType() == BoundType::Exact) {
			if (root) {
				bestMove = ttEntryRes->bestMove;
			}
			return ttEntryRes->eval;
		}
	}

	int16_t moveIndices[256];
	orderMoves(moves, movesCnt, moveIndices, ttBestMove, bestMoveAfterNull);
	//Prob cut
	const int16_t probCutBeta = beta + 2 * pieceValue[PAWN];
	constexpr int8_t probCutDepthR = 4;
	if (!pvNode && depth > probCutDepthR && !(foundTTEntry && ttEntryRes->depth >= depth - probCutDepthR) && !abCloseToMate) {
		for (int16_t i = 0; i < movesCnt; i++) {
			if (i != movesCnt - 1) {
				_mm_prefetch((const char*)&tt.chunk[pos->getZHashIfMoveMade(moves[moveIndices[i + 1]]) >> tt.shRVal], _MM_HINT_T1);
			}
			const int16_t curMove = moves[moveIndices[i]];
			//Make move
			const int8_t capturedPieceIdx = pos->makeMove(curMove);//int8_t declared is used to undo the move
			movesHistory.push(curMove);
			//Search
			int16_t res = -searchOnlyCaptures(-probCutBeta, -probCutBeta + 1);
			if (res >= probCutBeta) {
				res = -search<NonPV>(depth - probCutDepthR, -probCutBeta, -probCutBeta + 1);
			}
			//Undo move
			pos->undoMove(curMove, capturedPieceIdx, bitmaskCastling, possibleEnPassant);
			movesHistory.pop();
			if (res >= probCutBeta) {
				ttEntryRes->init(pos->zHash, curMove, res, depth - probCutDepthR, tt.gen, LowBound);
				return res;
			}
		}
	}

	//Search moves
	for (int16_t i = 0; i < movesCnt; i++) {
		//Prefetch next move in tt
		if (i != movesCnt - 1) {
			_mm_prefetch((const char*)&tt.chunk[pos->getZHashIfMoveMade(moves[moveIndices[i + 1]]) >> tt.shRVal], _MM_HINT_T1);
		}
		const int16_t curMove = moves[moveIndices[i]];
		//Reverse futility pruning (i think); https://www.chessprogramming.org/Reverse_Futility_Pruning
		if (depth <= 2) {
			int16_t evalIncrease = 0;
			if (pos->m_pieceOnTile[getEndPos(curMove)] != nullptr) {
				evalIncrease += pieceValue[pos->m_pieceOnTile[getEndPos(curMove)]->type];
			}
			if (getPromotionPiece(curMove) != PieceType::UNDEF) {
				evalIncrease += pieceValue[getPromotionPiece(curMove)];
			}
			if (depth == 2 && eval + evalIncrease - pieceValue[QUEEN] - 2 * pieceValue[PAWN]/*safety margin*/ >= beta) {
				return eval + evalIncrease - pieceValue[QUEEN] - 2 * pieceValue[PAWN];
			}
			if (depth == 1 && eval + evalIncrease + 2 * pieceValue[PAWN]/*safety margin*/ <= alpha) {
				break;
			}
		}

		//Make move
		const int8_t capturedPieceIdx = pos->makeMove(curMove);//int8_t declared is used to undo the move
		movesHistory.push(curMove);

		//Search
		int16_t res = 0;
		if (i == 0 || depth <= 2) {
			res = -search<(pvNode ? PV : NonPV)>(depth - 1, -beta, -alpha);
		} else {
			//Perform zero window search
			//https://www.chessprogramming.org/Principal_Variation_Search#ZWS
			//https://www.chessprogramming.org/Null_Window
			inScout++;
			res = -search<NonPV>(depth - 1, -alpha - 1, -alpha);
			inScout--;
			if (res > alpha && res < beta && beta - alpha > 1) {
				res = -search<(pvNode ? PV : NonPV)>(depth - 1, -beta, -res);
			}
		}

		//Undo move
		pos->undoMove(curMove, capturedPieceIdx, bitmaskCastling, possibleEnPassant);
		movesHistory.pop();

		if (Clock::now() >= searchEndTime) {
			return 0;
		}

		//If mate ? decrease value with depth
		if (res > pieceValue[KING] - MAX_PLY_MATE) {
			res -= 2;
		}
		//Alpha-beta cutoff
		if (res >= beta) {
			ttEntryRes->init(pos->zHash, curMove, res, depth, tt.gen, BoundType::LowBound);
			if (root) {
				pos->m_legalMovesCnt = movesCnt;
				bestMove = curMove;
			}
			const bool isCapture = pos->m_pieceOnTile[getEndPos(curMove)] != nullptr;
			if (!isCapture) {
				//Update history heuristic
				int* const hh = &historyHeuristic[pos->m_blackToMove][pos->m_pieceOnTile[getStartPos(curMove)]->type][getEndPos(curMove)];
				int clampedBonus = std::clamp(depth * depth, -MAX_HISTORY, MAX_HISTORY);
				*hh += clampedBonus - *hh * clampedBonus / MAX_HISTORY;
				//Update counter moves heuristic
				if (!movesHistory.empty() && movesHistory.top() != nullMove) {
					const int8_t lastMoveEnd = getEndPos(movesHistory.top());
					int16_t* const cMove = &counterMove[!pos->m_blackToMove][pos->m_pieceOnTile[lastMoveEnd]->type][lastMoveEnd];
					*cMove = curMove;
				}
				//Update killers
				if (killers[movesHistory.size()][0] == nullMove) {
					killers[movesHistory.size()][0] = curMove;
				} else {
					killers[movesHistory.size()][1] = curMove;
				}
			}
			return res;
		}
		//Best move
		if (res > bestValue) {
			if (root) {
				bestMove = curMove;
			}
			curBestMove = curMove;
			bestValue = res;
			if (res > alpha) {
				alpha = res;
				failLow = 0;
			}
		}
		//Uncertainty cutoffs
		if (((inScout && i > 6) || (inNullMoveSearch <= 2 && i > 8)) && 
			(pos->m_pieceOnTile[getEndPos(curMove)] == nullptr || !givesCheck(curMove))) {

			return alpha;
		}
	}
	if (root) {
		pos->m_legalMovesCnt = movesCnt;
	}
	//No alpha-beta cutoff occured
	//Don't overwrite best move with null move
	if (foundTTEntry && curBestMove == nullMove) {
		curBestMove = ttEntryRes->bestMove;
	}
	ttEntryRes->init(pos->zHash, curBestMove, bestValue, depth, tt.gen, (failLow ? BoundType::HighBound : BoundType::Exact));
	return alpha;
}

//https://www.chessprogramming.org/Quiescence_Search
inline int16_t AI::searchOnlyCaptures(int16_t alpha, int16_t beta) {
	if (Clock::now() >= searchEndTime) {
		return 0;
	}
	//Don't force player to capture if it is worse for him
	//(example: don't force Qxa3 if then there is bxa3)
	int16_t staticEval = pos->evaluate();
	//When going to cut, don't tt.find, because its expensive
	if (staticEval >= beta) {
		return staticEval;
	}
	//If even capturing a queen can't improve position
	if (staticEval + pieceValue[QUEEN] + pieceValue[PAWN]/*safety margin*/ <= alpha) {
		return alpha;
	}
	if (staticEval - pieceValue[QUEEN] - pieceValue[PAWN]/*safety margin*/ >= beta) {
		return staticEval - pieceValue[QUEEN] - pieceValue[PAWN];
	}
	//Fail low means no move gives a score > alpha
	//Starts with 1 and is set to 0 if a score > alpha is achieved
	bool failLow = 1;

	int16_t moves[256];
	pos->updateLegalMoves<1>(moves, true);
	const int16_t movesCnt = pos->m_legalMovesCnt;
	const int8_t bitmaskCastling = pos->m_bitmaskCastling;//Used for undo-ing moves
	const int8_t possibleEnPassant = pos->m_possibleEnPassant;//Used for undo-ing moves
	//Null move; https://www.chessprogramming.org/Null_Move_Pruning
	if (!pos->friendlyInCheck && !inNullMoveSearch) {
		//Make null move
		pos->makeNullMove();
		movesHistory.push(nullMove);
		inNullMoveSearch++;
		//Search
		int16_t nullRes = -searchOnlyCaptures(-beta, -beta + 1);
		//Undo null move
		pos->undoNullMove(possibleEnPassant);
		movesHistory.pop();
		inNullMoveSearch--;
		//Prune
		if (nullRes >= beta) {
			//Increment to not allow null move pruning in verification search
			inNullMoveSearch += 3;
			int16_t verification = searchOnlyCaptures(beta, beta + 1);
			inNullMoveSearch -= 3;
			if (verification >= beta) {
				return verification;
			}
		}
	}
	//Check tt
	bool foundTTEntry = 0;
	TTEntry* ttEntryRes = tt.find(pos->zHash, foundTTEntry);
	//Anyting written in tt will already contain all captures, so don't check depth
	if (foundTTEntry) {
		ttEntryRes->setGen(tt.gen);
		//If lower bound
		if (ttEntryRes->boundType() == BoundType::LowBound) {
			staticEval = max(staticEval, ttEntryRes->eval);
		}
		//Higher bound
		if (ttEntryRes->boundType() == BoundType::HighBound) {
			//We have guaranteed a better position
			if (ttEntryRes->eval <= alpha) {
				return alpha;
			}
		}
		//If exact bound => return eval
		if (ttEntryRes->boundType() == BoundType::Exact) {
			return ttEntryRes->eval;
		}
	}

	if (staticEval >= beta) {
		return staticEval;
	}
	if (staticEval > alpha) {
		alpha = staticEval;
		failLow = 0;
	}
	
	int16_t moveIndices[256];
	orderMoves(moves, movesCnt, moveIndices, (foundTTEntry ? ttEntryRes->bestMove : nullMove), nullMove);

	int16_t curBestMove = nullMove;
	for (int16_t i = 0; i < movesCnt; i++) {
		//Prefetch next move
		if (i != movesCnt - 1) {
			_mm_prefetch((const char*)&tt.chunk[pos->getZHashIfMoveMade(moves[moveIndices[i + 1]]) >> tt.shRVal], _MM_HINT_T1);
		}
		const int16_t curMove = moves[moveIndices[i]];
		//If can't improve alpha, don't search move; This is (i think) delta pruning
		//https://www.chessprogramming.org/Delta_Pruning
		int16_t evalIncrease = 0;
		if (pos->m_pieceOnTile[getEndPos(curMove)] != nullptr) {
			//Capture
			evalIncrease += pieceValue[pos->m_pieceOnTile[getEndPos(curMove)]->type];
		}
		if (getPromotionPiece(curMove) != PieceType::UNDEF) {
			//Promotion
			evalIncrease += pieceValue[getPromotionPiece(curMove)];
		}
		if (staticEval + evalIncrease - pieceValue[QUEEN] - pieceValue[PAWN]/*safety margin*/ >= beta) {
			return staticEval + evalIncrease - pieceValue[QUEEN] - pieceValue[PAWN];
		}
		if (staticEval + evalIncrease + pieceValue[PAWN]/*safety margin*/ <= alpha) {
			continue;
		}

		//Make move
		int8_t capturedPieceIdx = pos->makeMove(curMove);//int8_t declared is used to undo the move
		movesHistory.push(curMove);
		//Search
		int16_t res = 0;
		if (i == 0) {
			res = -searchOnlyCaptures(-beta, -alpha);
		} else {
			//Perftorm zero window search
			//https://www.chessprogramming.org/Principal_Variation_Search#ZWS
			//https://www.chessprogramming.org/Null_Window
			inScout++;
			res = -searchOnlyCaptures(-alpha - 1, -alpha);
			inScout--;
			if (res > alpha && beta - alpha > 1) {
				res = -searchOnlyCaptures(-beta, -alpha);
			}
		}
		//Undo move
		pos->undoMove(curMove, capturedPieceIdx, bitmaskCastling, possibleEnPassant);
		movesHistory.pop();

		if (Clock::now() >= searchEndTime) {
			return 0;
		}

		if (res >= beta) {
			//Don't replace ttEntry with garbage one useful only for qsearch
			if (!foundTTEntry || 
				//Unless found a qsearch entry with a less useful lower bound
				(foundTTEntry && ttEntryRes->depth == 0 && ttEntryRes->boundType() == LowBound && ttEntryRes->eval < res)) {
				
				ttEntryRes->init(pos->zHash, curMove, res, 0, tt.gen, BoundType::LowBound);
			}
			return res;
		}
		if (res > alpha) {
			alpha = res;
			failLow = 0;
			curBestMove = curMove;
		}
		//Uncertainty cutoffs
		if ((inScout && i > 2) || (inNullMoveSearch <= 2 && i > 4)) {

			return alpha;
		}
	}
	//Don't replace ttEntry with garbage one useful only for qsearch
	if (!foundTTEntry || 
		(foundTTEntry && ttEntryRes->depth == 0 && //Unless found a qsearch entry and
		//we have exact or we have a better high bound
		(!failLow || (ttEntryRes->boundType() == HighBound && ttEntryRes->eval > alpha)))) {
		
		
		//Here can write high bound if failLow and exact otherwise, because 
		//it only affect qsearch, because in main search we check if
		//ttEntry->depth >= depth, and here we write with depth = 0
		ttEntryRes->init(pos->zHash, curBestMove, alpha, 0, tt.gen, (failLow ? BoundType::HighBound : BoundType::Exact));
	}
	return alpha;
}

inline void AI::orderMoves(int16_t moves[256], int16_t movesCnt, int16_t* indices, int16_t ttBestMove, int16_t bestMoveAfterNull) {
	const uint64_t allPcs = pos->m_whiteAllPiecesBitboard | pos->m_blackAllPiecesBitboard;
	const int8_t enemyKingPos = (pos->m_blackToMove ? pos->m_whitePiece[0]->pos : pos->m_blackPiece[0]->pos);

	int8_t bmAfterNullSt = getStartPos(bestMoveAfterNull);
	int8_t bmAfterNullEnd = getEndPos(bestMoveAfterNull);
	uint64_t nullDefenders[6] = { 0, 0, 0, 0, 0, 0 };
	if (bestMoveAfterNull != nullMove) {
		nullDefenders[KING]		= attacks<KING>		(bmAfterNullEnd, allPcs);
		nullDefenders[BISHOP]	= attacks<BISHOP>	(bmAfterNullEnd, allPcs);
		nullDefenders[KNIGHT]	= attacks<KNIGHT>	(bmAfterNullEnd, allPcs);
		nullDefenders[ROOK]		= attacks<ROOK>		(bmAfterNullEnd, allPcs);
		nullDefenders[PAWN]		= pawnAtt<!pos->m_blackToMove>(bmAfterNullEnd);
		nullDefenders[QUEEN]	= nullDefenders[BISHOP] | nullDefenders[ROOK];
	}
	
	for (int16_t i = 0; i < movesCnt; i++) {
		//Note: here we only guess how good a move is, so we can afford to make
		//guesses overinflated since the only thing that matters is their relative values
		const int16_t curMove = moves[i];
		const int8_t stPos = getStartPos(curMove), endPos = getEndPos(curMove);
		const PieceType pt = pos->m_pieceOnTile[stPos]->type;
		int curGuess = 0;
		indices[i] = i;
		//Favor move in tt
		if (curMove == ttBestMove) {
			evalGuess[i] = 0x7FFF;//<- just a sufficiently large number
			continue;
		}

		//Add bonuses; Have to have if(black) here, because template doesn't accept it as argument
		if (pos->m_blackToMove) {
			curGuess += (getSqBonus<1>(pt, endPos) - getSqBonus<1>(pt, stPos)) * 8 / 5;
		} else {
			curGuess += (getSqBonus<0>(pt, endPos) - getSqBonus<0>(pt, stPos)) * 8 / 5;
		}
		//Add history heuristic
		curGuess += historyHeuristic[pos->m_blackToMove][pt][endPos];
		//Add counter move heuristic
		if (!movesHistory.empty() && movesHistory.top() != nullMove) {
			const int8_t lastMoveEnd = getEndPos(movesHistory.top());
			if (curMove == counterMove[!pos->m_blackToMove][pos->m_pieceOnTile[lastMoveEnd]->type][lastMoveEnd]) {
				curGuess += COUNTER_MOVE_BONUS;
			}
		}
		//Add bonus for evading opponent threats
		//If defending
		if (nullDefenders[pt] && (1ULL << endPos)) {
			curGuess += NULL_MOVE_DEFEND_BONUS;
		}
		//If escaping from opponent threat
		if (stPos == bmAfterNullEnd) {
			curGuess += pieceValue[pt] / 32;
		}
		//If playing counter move to opponent threat
		if (bestMoveAfterNull != nullMove) {
			if (curMove == counterMove[!pos->m_blackToMove][pos->m_pieceOnTile[bmAfterNullSt]->type][bmAfterNullEnd]) {
				curGuess += COUNTER_MOVE_BONUS / 3;
			}
		}
		//Add killers
		if (curMove == killers[movesHistory.size()][0] ||
			curMove == killers[movesHistory.size()][1]) {

			curGuess += KILLER_BONUS;
		}

		//Favor captures
		if (pos->m_pieceOnTile[getEndPos(curMove)] != nullptr) {
			//13 * capturedPieceVal to prioritise captures before non-captures and 
			//to incentivise capturing more valuable pieces; We chosse 13, because
			//13 is the lowest number above how much pawns a queen is worth
			
			//pieceValue[KING] = inf; so king captures would be placed last; to prevent this add if (pt == KING)
			if (pt == KING) {
				curGuess += 13 * pieceValue[pos->m_pieceOnTile[endPos]->type] - pieceValue[QUEEN] * 3 / 2;
			} else {
				curGuess += 13 * pieceValue[pos->m_pieceOnTile[endPos]->type] - pieceValue[pt];
			}
		}
		//If tile attacked by opponent pawn
		if ((1ULL << endPos) & pos->enemyPawnAttacksBitmask) {
			curGuess -= 4 * pieceValue[pt];
		}

		//Promoting is good
		if (getPromotionPiece(curMove) != UNDEF) {
			//Multiply by 15, because when capturing we multiply 
			//by 13 and promoting is more likely to be good (I think)
			curGuess += 15 * pieceValue[getPromotionPiece(curMove)];
		}
		//Favor checks slightly
		if (givesCheck(curMove)) {
			//Add pieceValue[piece to make move] / 32, because generally checks with
			//queens and rooks are better (and there is a higher chance for a fork)
			curGuess += CHECK_BONUS + (pieceValue[pt] / 32);
		}

		//Write in guess array that will later be used to sort the moves
		evalGuess[i] = curGuess;
	}
	//Use indices to "link" evalGuess and pos->m_legalMove together because use std::sort
	std::sort(indices, indices + movesCnt,
		[this](const int16_t idx1, const int16_t idx2) {
			return evalGuess[idx1] > evalGuess[idx2];
		}
	);
}