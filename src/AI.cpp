#include "AI.h"
const int AI::MAX_PLY_MATE = 256;
const int AI::CHECK_BONUS = 125;
const int AI::MAX_HISTORY = 250;
const int AI::COUNTER_MOVE_BONUS = 192;
const int AI::KILLER_BONUS = 350;
const int AI::NULL_MOVE_DEFEND_BONUS = 30;
const int AI::NULL_MOVE_MATE_DEFEND_BONUS = 420;


AI::AI() {
	pos = nullptr;
	bestMove = nullMove;
	evalGuess = new int[MAX_LEGAL_MOVES]();
	movesHistory.init(256);
	searchEndTime = 0;
	inNullMoveSearch = 0;
	inScout = 0;
	curExtentions = 0;
}

AI::~AI() {
	//if (pos != nullptr) {
	//	delete pos;
	//}
	if (evalGuess != nullptr) {
		delete[] evalGuess;
	}
}

void AI::initPos(Position* pos_) {
	pos = pos_;
	resetHistory();
}

int16_t AI::startSearch(uint64_t timeToSearch, bool printRes) {
	searchEndTime = Clock::now() + timeToSearch;
	//Reset killers
	for (uint8_t i = 0; i < 128; i++) {
		killers[i][0] = killers[i][1] = nullMove;
	}
	tt.newGen();
	return iterativeDeepening(127, printRes);
}

void AI::resetHistory() {
	//Reset hh and cmh
	for (int8_t i = 0; i < 6; i++) {
		for (int8_t j = 0; j < 64; j++) {
			historyHeuristic[0][i][j] = 0;
			historyHeuristic[1][i][j] = 0;
			counterMove[0][i][j] = nullMove;
			counterMove[1][i][j] = nullMove;
			for (int8_t k = 0; k < 6; k++) {
				captureHistory[0][i][j][k] = 0;
				captureHistory[1][i][j][k] = 0;
			}
		}
	}
	movesHistory.clear();
	inNullMoveSearch = 0;
	inScout = 0;
	for (uint8_t i = 0; i < 128; i++) {
		killers[i][0] = killers[i][1] = nullMove;
		threats[i] = nullMove;
	}
}

void AI::updateHistoryNewSearch() {
	for (int8_t i = 0; i < 6; i++) {
		for (int8_t j = 0; j < 64; j++) {
			//Don't fully reset hh, but reduce it
			historyHeuristic[0][i][j] /= 2;
			historyHeuristic[1][i][j] /= 2;
			for (int8_t k = 0; k < 6; k++) {
				captureHistory[0][i][j][k] *= 2;
				captureHistory[0][i][j][k] /= 3;
				captureHistory[1][i][j][k] *= 2;
				captureHistory[1][i][j][k] /= 3;
			}
			//Dont reset counter moves, because good chance that they will remain good
		}
	}
	movesHistory.clear();
	inScout = 0;
	inNullMoveSearch = 0;
	curExtentions = 0;
}


#include <iomanip>//setw() when outputing info
inline int16_t AI::iterativeDeepening(int8_t depth, bool printRes) {
	bool foundRootInTT = false;
	TTEntry* ttRoot = tt.find(pos->zHash, foundRootInTT);

	int16_t rootMoves[256], rootMovesCnt = 0, alpha, beta;
	for (int i = 0; i < 256; i++) {
		rootMoves[i] = nullMove;
		rootMovesEval[i] = -pieceValue[KING];
		rootMoveIndices[i] = i;
	}
	pos->updateLegalMoves<0>(rootMoves, true);
	rootMovesCnt = pos->m_legalMovesCnt;
	orderMoves(rootMoves, rootMovesCnt, rootMoveIndices, (foundRootInTT ? nullMove : ttRoot->bestMove), nullMove, 0);

	int16_t eval = pos->evaluate();
	int16_t lastEval = 0;
	for (int8_t i = min<int8_t>(depth, 2); i <= depth; i++) { 
		if (i < 0 || i == 127) {
			break;
		}
		//Don't oversaturate history heuristic moves that are near the root
		updateHistoryNewSearch();

		//Aspiration window formula taken from stockfish
		int16_t delta = 9, curEval = 0;
		alpha = max<int16_t>(eval - delta, -pieceValue[KING]);
		beta = min<int16_t>(eval + delta, pieceValue[KING]);
		while (1) {
			if (Clock::now() >= searchEndTime) {
				break;
			}
			
			curEval = search<Root>(i, alpha, beta, false);
			if (curEval <= alpha) {
				beta = (alpha + beta) / 2;
				alpha = max<int16_t>(curEval - delta, -pieceValue[KING]);
			} else {
				if (curEval >= beta) {
					beta = min<int16_t>(curEval + delta, pieceValue[KING]);
				} else {
					break;
				}
			}
			if (delta >= pieceValue[QUEEN]) {
				curEval = search<Root>(i, -pieceValue[KING] - 1, pieceValue[KING] + 1, false);
				break;
			}
			delta += delta / 3;
		}

		//Exit if ran out of time
		if (Clock::now() >= searchEndTime) {
			if (printRes) {
				std::cout << "Search ended { Eval: " << eval << "; Best move: ";
				printName(bestMove);
				std::cout << "; }\n";
			}
			return eval;
		}
		//Print info
		//std::setw(x) makes the next output have a min width of x (it just adds spaces)
		if (printRes) {
			std::cout << "Depth " << std::setw(2) << (int)i << ": { Eval = " << std::setw(5) << curEval << "; Best move = ";
			printName(bestMove);
			std::cout << "; }; Time remaining: ";
			std::cout << std::max<int64_t>(0, searchEndTime - Clock::now()) << '\n';
		}
		std::stable_sort(rootMoveIndices, rootMoveIndices + rootMovesCnt,
			[this](const int16_t idx1, const int16_t idx2) {
				return rootMovesEval[idx1] > rootMovesEval[idx2];
			}
		);
		lastEval = eval;
		eval = curEval;
	}
	return eval;
}

template<NodeType nodeType>
inline int16_t AI::search(int8_t depth, int16_t alpha, int16_t beta, bool cutNode) {
	if (Clock::now() >= searchEndTime) {
		return 0;
	}
	//Handle draws
	if (pos->drawMan.checkForRule50()) {
		return 0;
	}
	if (pos->hasInsufMaterial(pos->m_blackToMove)) {
		beta = min<int16_t>(beta, 0);
	}
	if (pos->hasInsufMaterial(!pos->m_blackToMove)) {
		alpha = max<int16_t>(alpha, 0);
	}
	if (alpha >= beta) {
		return alpha;
	}
	if (pos->drawMan.checkForRep()) {
		return 0;
	}
	//qsearch
	if (depth <= 0) {
		return searchOnlyCaptures(alpha, beta);
	}
	constexpr bool root = (nodeType == Root);
	constexpr bool pvNode = (nodeType != NonPV);

	const int8_t bitmaskCastling = pos->m_bitmaskCastling;//Used for undo-ing moves
	const int8_t possibleEnPassant = pos->m_possibleEnPassant;//Used for undo-ing moves
	const int8_t r50count = pos->drawMan.rule50count;//Used for undo-ing moves
	int16_t eval = pos->evaluate();
	int16_t movesCnt, moves[256], bestMoveAfterNull = nullMove, curBestMove = nullMove;
	int16_t bestValue = -pieceValue[KING], nmScoreDelta = 0;
	const bool lastWasNull = (!movesHistory.empty() && movesHistory.top() == nullMove);
	const bool abCloseToMate =	abs((abs(alpha) - pieceValue[KING])) <= MAX_PLY_MATE ||
								abs((abs(beta) - pieceValue[KING])) <= MAX_PLY_MATE;
	//Fail low means no move gives a score > alpha
	//Starts with 1 and is set to 0 if a score > alpha is achieved
	bool failLow = 1, foundTTEntry = 0;


	//Mate distance pruning
	if (!root) {
		alpha = max<int16_t>(alpha, -pieceValue[KING] + 2 * movesHistory.size());
		beta = min<int16_t>(beta, pieceValue[KING] - 2 * movesHistory.size());
		if (alpha >= beta) {
			return alpha;
		}
	}

	pos->updateLegalMoves<0>(moves, true);
	movesCnt = pos->m_legalMovesCnt;
	//Handle mate and stalemate
	if (movesCnt == 0) {
		if (pos->friendlyInCheck) {
			if (movesHistory.size() != 0 && movesHistory.top() != nullMove) {
				int8_t lastMoveEnd = getEndPos(movesHistory.top());
				int* hh = &historyHeuristic[!pos->m_blackToMove][pos->m_pieceOnTile[lastMoveEnd]->type][lastMoveEnd];
				*hh += pieceValue[PAWN] * 5 / 4 - *hh * pieceValue[PAWN] * 5 / 4 / MAX_HISTORY;
			}
			return -pieceValue[KING];
		}
		return 0;
	}
	prefetch((const char*)&tt.chunk[pos->zHash >> tt.shRVal].entry);

	//Razoring <https://www.chessprogramming.org/Razoring>
	//Taken from stockfish
	if (!abCloseToMate && eval < alpha - 319 - 151 * (depth * depth)) {
		int16_t value = searchOnlyCaptures(alpha - 1, alpha);
		if (value < alpha) {
			return alpha;
		}
	}
	//Limited razoring
	if (!abCloseToMate && depth == 3 && eval + pieceValue[PAWN] * 2 + pvNode * 27 <= alpha &&
		(pos->m_blackToMove ? pos->m_whiteTotalPiecesCnt : pos->m_blackTotalPiecesCnt) > 3) {

		depth--;
	}

	//Futility pruning <https://www.chessprogramming.org/Futility_Pruning>
	if (!pos->friendlyInCheck && depth <= 9 && !abCloseToMate) {
		if (eval - pieceValue[PAWN] * depth / 2 >= beta) {
			return (eval + beta) / 2;
		}
	}

	if (cutNode && eval - pieceValue[pos->highestPiece(pos->m_blackToMove)] > beta && depth <= 3) {
		//depth--;
	}

	//Null move pruning <https://www.chessprogramming.org/Null_Move_Pruning>
	if (!pvNode && !pos->friendlyInCheck && pos->hasNonPawnPiece(pos->m_blackToMove) && !abCloseToMate && eval >= beta) {
		//Don't chain a lot of null moves, but if last was null and in 
		//only one null move search, do null movee to detect zugzwang
		if (inNullMoveSearch == lastWasNull) {
			//Depth reduction formula taken from stockfish and changed a little
			int8_t nullMoveDepthR = std::clamp(int(eval - beta) / pieceValue[PAWN], 0, 5) + 4 + depth / 3;
			if (lastWasNull) {
				//If last was null check another null move to detect zugzwang without reducing depth
				nullMoveDepthR = 0;
			}
			//Make move
			pos->makeNullMove();
			movesHistory.push(nullMove);
			inNullMoveSearch++;
			//Search
			int16_t nullRes = -searchOnlyCaptures(-beta, -beta + 1);
			if (nullRes < beta) {
				nullRes = -search<NonPV>(depth - nullMoveDepthR, -beta, -beta + 1, !cutNode);
			}
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
				//MovesHistory won't be empty, because we only do it in nonPv nodes and root is pv
				int16_t lastMoveEnd = getEndPos(movesHistory.top());
				if (depth - nullMoveDepthR <= 0) {
					return nullRes;
				}
				//Don't do null moves in verification search
				inNullMoveSearch += 3; inScout++;
				int16_t verification = search<NonPV>(depth - nullMoveDepthR, beta - 1, beta, true);
				inNullMoveSearch -= 3; inScout--;
				if (verification >= beta) {
					//Update hh
					if (!lastWasNull) {
						int* hh = &historyHeuristic[!pos->m_blackToMove][pos->m_pieceOnTile[lastMoveEnd]->type][lastMoveEnd];
						int clampedBonus = -std::clamp((depth - nullMoveDepthR) * (depth - nullMoveDepthR), -MAX_HISTORY, MAX_HISTORY);
						*hh += (clampedBonus + *hh * clampedBonus / MAX_HISTORY) / 5;
					}
					killers[movesHistory.size() + 2][0] = killers[movesHistory.size() + 2][1] = nullMove;
					return verification;
				}
			}
			//Detect when doing nothing will result in mate
			nmScoreDelta = eval - nullRes;
			if (nullRes <= -pieceValue[KING] + MAX_PLY_MATE) {
				nmScoreDelta = pieceValue[KING];
			}
		}
	}
	threats[movesHistory.size()] = bestMoveAfterNull;

	//Probe tt
	TTEntry* ttEntryRes = tt.find(pos->zHash, foundTTEntry);
	int16_t ttBestMove = (foundTTEntry && ttEntryRes->depth != 0 ? ttEntryRes->bestMove : nullMove);
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
			return max(alpha, ttEntryRes->eval);
		}
	}

	//Internal iterative reductions (taken from stockfish)
	//<https://www.chessprogramming.org/Internal_Iterative_Reductions>
	if (depth >= 5 && pvNode && ttBestMove == nullMove) {
		depth -= 2 + (foundTTEntry && ttEntryRes->depth >= depth);
	}

	int16_t moveIndices[256];
	//if (!(root && depth >= 8)) {
		orderMoves(moves, movesCnt, moveIndices, ttBestMove, bestMoveAfterNull, nmScoreDelta);
	//} else {
	//	memcpy(moveIndices, rootMoveIndices, movesCnt * sizeof(int16_t));
	//}

	//Milti cut https://www.chessprogramming.org/Multi-Cut
	const int8_t multiCutDepthR = depth / 3 + 2;
	//inScout impiles its also not a PV node
	if (cutNode && inScout && !abCloseToMate) {
		int16_t curCutNodes = 0;
		const int16_t movesToSearch = min<int16_t>(8, movesCnt), cutNodesToQuit = 2;
		for (int8_t i = 0; i < movesToSearch; i++) {
			const int16_t curMove = moves[moveIndices[i]];
			const int8_t moveSt = getStartPos(curMove), moveEnd = getEndPos(curMove);
			//Make move
			const int8_t capturedPieceIdx = pos->makeMove(curMove);//int8_t declared is used to undo the move
			movesHistory.push(curMove);
			//Search
			int16_t res = -searchOnlyCaptures(-beta, -beta + 1);
			if (res >= beta && depth - 1 > multiCutDepthR) {
				res = -search<NonPV>(depth - 1 - multiCutDepthR, -beta, -beta + 1, !cutNode);
			}
			//Undo move
			pos->undoMove(curMove, capturedPieceIdx, bitmaskCastling, possibleEnPassant, r50count);
			movesHistory.pop();
			if (res >= beta) {
				if (++curCutNodes == cutNodesToQuit) {
					int clampedBonus = std::clamp((depth - multiCutDepthR) * (depth - multiCutDepthR), -MAX_HISTORY, MAX_HISTORY);
					if (!pos->isCapture(curMove)) {
						int* hh = &historyHeuristic[pos->m_blackToMove][pos->m_pieceOnTile[moveSt]->type][moveEnd];
						*hh += clampedBonus - *hh * clampedBonus / MAX_HISTORY;
					} else {
						int* ch = &captureHistory[pos->m_blackToMove][pos->m_pieceOnTile[moveSt]->type][moveEnd][pos->m_pieceOnTile[moveEnd]->type];
						*ch += clampedBonus - *ch * clampedBonus / MAX_HISTORY;
					}
					ttEntryRes->init(pos->zHash, curMove, res, depth - multiCutDepthR, tt.gen, LowBound);
					killers[movesHistory.size() + 2][0] = killers[movesHistory.size() + 2][1] = nullMove;
					return res;
				}
			}
		}
	}

	//ProbCut https://www.chessprogramming.org/ProbCut
	int16_t probCutBeta = beta + 87;
	const int8_t probCutDepthR = 3;
	if (!pvNode && depth > probCutDepthR && !abCloseToMate &&
		!(foundTTEntry && ttEntryRes->depth >= depth - probCutDepthR && ttEntryRes->eval < probCutBeta)) {
		for (int16_t i = 0; i < movesCnt; i++) {
			const int16_t curMove = moves[moveIndices[i]];
			const int8_t moveSt = getStartPos(curMove), moveEnd = getEndPos(curMove);
			//Make move
			const int8_t capturedPieceIdx = pos->makeMove(curMove);//int8_t declared is used to undo the move
			movesHistory.push(curMove);
			//Search
			int16_t res = -searchOnlyCaptures(-probCutBeta, -probCutBeta + 1);
			if (res >= probCutBeta) {
				res = -search<NonPV>(depth - probCutDepthR - 1, -probCutBeta, -probCutBeta + 1, !cutNode);
			}
			//Undo move
			pos->undoMove(curMove, capturedPieceIdx, bitmaskCastling, possibleEnPassant, r50count);
			movesHistory.pop();
			if (res >= probCutBeta) {
				//Update history heuristic
				int clampedBonus = std::clamp((depth - probCutDepthR) * (depth - probCutDepthR), -MAX_HISTORY, MAX_HISTORY);
				if (!pos->isCapture(curMove)) {
					int* hh = &historyHeuristic[pos->m_blackToMove][pos->m_pieceOnTile[moveSt]->type][moveEnd];
					*hh += clampedBonus - *hh * clampedBonus / MAX_HISTORY;
				} else {
					int* ch = &captureHistory[pos->m_blackToMove][pos->m_pieceOnTile[moveSt]->type][moveEnd][pos->m_pieceOnTile[moveEnd]->type];
					*ch += clampedBonus - *ch * clampedBonus / MAX_HISTORY;
				}
				ttEntryRes->init(pos->zHash, curMove, res, depth - probCutDepthR, tt.gen, LowBound);
				killers[movesHistory.size() + 2][0] = killers[movesHistory.size() + 2][1] = nullMove;
				return res;
			}
		}
	}

	//Search moves
	for (int16_t i = 0; i < movesCnt; i++) {
		const int16_t curMove = moves[moveIndices[i]];
		const int8_t moveSt = getStartPos(curMove), moveEnd = getEndPos(curMove);
		const bool quiet = pos->isCapture(curMove) && !isPromotion(curMove) && !givesCheck(curMove);
		//Uncertainty cutoffs https://www.chessprogramming.org/Uncertainty_Cut-Offs
		if ((inScout && i > 6) && !pos->isCapture(curMove) && !isPromotion(curMove) && !(givesCheck(curMove) && abCloseToMate)) {
			return alpha;
		}
		//Reverse futility pruning (i think); https://www.chessprogramming.org/Reverse_Futility_Pruning
		int16_t evalIncrease = 0;
		if (pos->isCapture(curMove)) {
			evalIncrease += pieceValue[pos->m_pieceOnTile[getEndPos(curMove)]->type];
		}
		if (getPromotionPiece(curMove) != PieceType::UNDEF) {
			evalIncrease += pieceValue[getPromotionPiece(curMove)];
		}
		if (depth == 2 && eval + evalIncrease - pieceValue[pos->highestPiece(pos->m_blackToMove)] - pieceValue[PAWN]/*safety margin*/ >= beta) {
			return eval + evalIncrease - pieceValue[pos->highestPiece(pos->m_blackToMove)];
		}
		if (depth == 1 && eval + evalIncrease + pieceValue[PAWN]/*safety margin*/ <= alpha) {
			break;
		}
		//Taken from stockfish and added conditions for hh, cMove and killers
		if (!root && !pos->friendlyInCheck && depth <= 8 && !abCloseToMate && quiet && 
			eval + evalIncrease + pieceValue[PAWN] * depth / 2 + (bestValue < eval - 57 ? 114 : 57) <= alpha) {
			const int8_t lastMoveEnd = getEndPos(movesHistory.top());

			int hh = historyHeuristic[pos->m_blackToMove][pos->m_pieceOnTile[moveSt]->type][moveEnd];
			int const cMove = (lastWasNull ? nullMove : counterMove[pos->m_blackToMove][pos->m_pieceOnTile[lastMoveEnd]->type][lastMoveEnd]);
			if (hh <= MAX_HISTORY / 8 && curMove != cMove && 
				curMove != killers[movesHistory.size()][0] && curMove != killers[movesHistory.size()][1]) {

				continue;
			}
		}

		//Make move
		const int8_t capturedPieceIdx = pos->makeMove(curMove);//int8_t declared is used to undo the move
		movesHistory.push(curMove);

		//Search
		int16_t res = 0;
		int8_t newDepth = depth, extention = 0;
		if (depth >= 2 && i > 1 + root && quiet) {
			//Late move reductions
			inScout++;
			res = -search<NonPV>((newDepth - 2 + pvNode) - 1, -alpha - 1, -alpha, true);
			inScout--;
			//If needs full search
			if (res > alpha) {
				res = -search<NonPV>(newDepth - 1, -alpha - 1, -alpha, !cutNode);
			}
		} else {
			//<https://www.chessprogramming.org/Check_Extensions>
			extention += (curExtentions <= 12 && givesCheck(curMove));

			curExtentions += extention;
			newDepth += extention;
			if (i == 0 && pvNode) {
				res = -search<PV>(newDepth - 1, -beta, -alpha, false);
			} else {
				//Perform zero window search
				//https://www.chessprogramming.org/Principal_Variation_Search#ZWS
				//https://www.chessprogramming.org/Null_Window
				inScout++;
				res = -search<NonPV>(newDepth - 1, -alpha - 1, -alpha, !cutNode);
				inScout--;
			}
		}
		if (res > alpha && pvNode && beta - alpha > 1) {
			res = -search<PV>(newDepth - 1, -beta, -alpha, false);
		}
		curExtentions -= extention;

		killers[movesHistory.size() + 2][0] = killers[movesHistory.size() + 2][1] = nullMove;

		//Undo move
		pos->undoMove(curMove, capturedPieceIdx, bitmaskCastling, possibleEnPassant, r50count);
		movesHistory.pop();

		if (Clock::now() >= searchEndTime) {
			return 0;
		}

		//If mate decrease value with depth
		if (res > pieceValue[KING] - MAX_PLY_MATE) {
			res -= 2;
		}
		if (root) {
			if (res > alpha) {
				const int16_t rmIdx = rootMoveIndices[i];
				rootMovesEval[rmIdx] = res;
			}
		}

		//Beta cutoff
		if (res >= beta) {
			ttEntryRes->init(pos->zHash, curMove, res, depth, tt.gen, BoundType::LowBound);
			int clampedBonus = std::clamp(depth * depth, -MAX_HISTORY, MAX_HISTORY);
			if (!pos->isCapture(curMove)) {
				//Update history heuristic
				int*  hh = &historyHeuristic[pos->m_blackToMove][pos->m_pieceOnTile[moveSt]->type][moveEnd];
				*hh += clampedBonus - *hh * clampedBonus / MAX_HISTORY;
				//Update counter moves heuristic
				if (!movesHistory.empty() && movesHistory.top() != nullMove) {
					const int8_t lastMoveEnd = getEndPos(movesHistory.top());
					int16_t* const cMove = &counterMove[!pos->m_blackToMove][pos->m_pieceOnTile[lastMoveEnd]->type][lastMoveEnd];
					*cMove = curMove;
				}
				//Update killers
				updateKillers(curMove);
			} else {
				int* ch = &captureHistory[pos->m_blackToMove][pos->m_pieceOnTile[moveSt]->type][moveEnd][pos->m_pieceOnTile[moveEnd]->type];
				*ch += clampedBonus - *ch * clampedBonus / MAX_HISTORY;
			}
			return res;
		} else {
			if (res <= alpha && pos->isCapture(curMove) && eval >= beta - 2 * pieceValue[PAWN]) {
				int* ch = &captureHistory[pos->m_blackToMove][pos->m_pieceOnTile[moveSt]->type][moveEnd][pos->m_pieceOnTile[moveEnd]->type];
				int bonus = -fastSqrt(pieceValue[pos->m_pieceOnTile[moveEnd]->type]) / (3 + 2 * (depth <= 2) + (depth <= 4));
				bonus = std::clamp(bonus, -MAX_HISTORY, MAX_HISTORY);
				*ch += bonus - *ch * abs(bonus) / MAX_HISTORY;
			}
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
	if (pos->hasInsufMaterial(pos->m_blackToMove)) {
		beta = min<int16_t>(beta, 0);
	}
	if (pos->hasInsufMaterial(!pos->m_blackToMove)) {
		alpha = max<int16_t>(alpha, 0);
	}
	if (alpha >= beta) {
		return alpha;
	}
	//Don't force player to capture if it is worse for him
	//(example: don't force Qxa3 if then there is bxa3)
	int16_t staticEval = pos->evaluate();
	//When going to cut, don't tt.find, because its expensive
	if (staticEval >= beta) {
		return staticEval;
	}
	//If even capturing a opponent's highest pieces can't improve position
	if (staticEval + pieceValue[pos->highestPiece(!pos->m_blackToMove)] + pieceValue[PAWN]/*safety margin*/ <= alpha) {
		return alpha;
	}
	//Fail low means no move gives a score > alpha
	//Starts with 1 and is set to 0 if a score > alpha is achieved
	bool failLow = 1;

	int16_t moves[64], moveIndices[64];
	pos->updateLegalMoves<1>(moves, true);
	const int16_t movesCnt = pos->m_legalMovesCnt;
	const int8_t bitmaskCastling = pos->m_bitmaskCastling;//Used for undo-ing moves
	const int8_t possibleEnPassant = pos->m_possibleEnPassant;//Used for undo-ing moves
	const int8_t r50count = pos->drawMan.rule50count;//Used for undo-ing moves
	//Probe tt
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
			return max(alpha, ttEntryRes->eval);
		}
	}

	if (staticEval >= beta) {
		return staticEval;
	}
	if (staticEval > alpha) {
		alpha = staticEval;
		failLow = 0;
	}


	if (pos->friendlyInCheck && pos->m_legalMovesCnt == 0) {
		pos->updateLegalMoves<0>(moves, true);
		if (pos->m_legalMovesCnt == 0) {
			return -pieceValue[KING];
		}
		return alpha;
	}

	orderMoves(moves, movesCnt, moveIndices, (foundTTEntry ? ttEntryRes->bestMove : nullMove), nullMove);

	int16_t curBestMove = nullMove;
	for (int16_t i = 0; i < movesCnt; i++) {
		//Uncertainty cutoffs https://www.chessprogramming.org/Uncertainty_Cut-Offs
		if (inScout && i > 2) {
			return alpha;
		}
		if (i != movesCnt - 1) {
			prefetch((const char*)&tt.chunk[pos->getZHashIfMoveMade(moves[moveIndices[i + 1]]) >> tt.shRVal]);
		}
		
		const int16_t curMove = moves[moveIndices[i]];
		const int16_t see = pos->SEE(curMove);
		//If can't improve alpha, don't search move; This is (i think) delta pruning
		//https://www.chessprogramming.org/Delta_Pruning
		int16_t evalIncrease = 0;
		if (pos->isCapture(curMove)) {
			evalIncrease += pieceValue[pos->m_pieceOnTile[getEndPos(curMove)]->type];
		}
		if (getPromotionPiece(curMove) != PieceType::UNDEF) {
			//Promotion
			evalIncrease += pieceValue[getPromotionPiece(curMove)];
		}
		if (staticEval + evalIncrease + pieceValue[PAWN]/*safety margin*/ <= alpha) {
			break;
		}
		//Chance of capture being good is only if discovered attack on queen
		if (see <= -pieceValue[ROOK]) {
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
		pos->undoMove(curMove, capturedPieceIdx, bitmaskCastling, possibleEnPassant, r50count);
		movesHistory.pop();

		if (Clock::now() >= searchEndTime) {
			return 0;
		}

		if (res >= beta) {
			//Don't replace ttEntry with garbage one useful only for qsearch
			if (!foundTTEntry ||
				//Unless found a qsearch entry with a less useful lower bound
				(foundTTEntry && ttEntryRes->depth <= 0 && ttEntryRes->boundType() == LowBound && ttEntryRes->eval < res)) {

				ttEntryRes->init(pos->zHash, curMove, res, 0, tt.gen, BoundType::LowBound);
			}
			return res;
		}
		if (res > alpha) {
			alpha = res;
			failLow = 0;
			curBestMove = curMove;
		}
	}
	//Don't replace ttEntry with garbage one useful only for qsearch
	if (!foundTTEntry ||
		(foundTTEntry && ttEntryRes->depth <= 0 && //Unless found a qsearch entry and
		//we have exact or we have a better high bound
		(!failLow || (ttEntryRes->boundType() == HighBound && ttEntryRes->eval > alpha)))) {

		//Here can write high bound if failLow and exact otherwise, because 
		//it only affect qsearch, because in main search we check if
		//ttEntry->depth >= depth, and here we write with depth = 0
		ttEntryRes->init(pos->zHash, curBestMove, alpha, 0, tt.gen, (failLow ? BoundType::HighBound : BoundType::Exact));
	}
	return alpha;
}

inline void AI::orderMoves	(int16_t* moves, int16_t movesCnt, int16_t* indices, 
							int16_t ttBestMove, int16_t bestMoveAfterNull, int16_t nmScoreDelta) {
	
	const uint64_t allPcs = pos->m_whiteAllPiecesBitboard | pos->m_blackAllPiecesBitboard;
	const int8_t enemyKingPos = (pos->m_blackToMove ? pos->m_whitePiece[0]->pos : pos->m_blackPiece[0]->pos);
	const int8_t friendlyKingPos = (pos->m_blackToMove ? pos->m_blackPiece[0]->pos : pos->m_whitePiece[0]->pos);

	int8_t bmAfterNullSt = (bestMoveAfterNull == nullMove ? -1 : getStartPos(bestMoveAfterNull));
	int8_t bmAfterNullEnd = (bestMoveAfterNull == nullMove ? -1 : getEndPos(bestMoveAfterNull));
	uint64_t nullDefenders[6] = { 0, 0, 0, 0, 0, 0 };
	if (bestMoveAfterNull != nullMove) {
		nullDefenders[KING]		= attacks<KING>		(bmAfterNullEnd, allPcs);
		nullDefenders[BISHOP]	= attacks<BISHOP>	(bmAfterNullEnd, allPcs);
		nullDefenders[KNIGHT]	= attacks<KNIGHT>	(bmAfterNullEnd, allPcs);
		nullDefenders[ROOK]		= attacks<ROOK>		(bmAfterNullEnd, allPcs);
		if (pos->m_blackToMove) { nullDefenders[PAWN] = pawnAttacks<0>(bmAfterNullEnd); }
		else					{ nullDefenders[PAWN] = pawnAttacks<1>(bmAfterNullEnd); }
		nullDefenders[QUEEN]	= nullDefenders[BISHOP] | nullDefenders[ROOK];
	}
	bool nmIsBigCapture = 0, mateIfNullMove = (nmScoreDelta >= pieceValue[KING] - MAX_PLY_MATE);
	bool nmIsCapture = (bestMoveAfterNull != nullMove && pos->isCapture(bestMoveAfterNull));
	if (bestMoveAfterNull != nullMove && nmIsCapture) {
		const PieceType capturingType = pos->m_pieceOnTile[bmAfterNullEnd]->type;
		const PieceType capturedType = pos->m_pieceOnTile[bmAfterNullSt]->type;
		if (!(capturingType == KNIGHT && capturedType == BISHOP)) {
			nmIsBigCapture = pieceValue[capturingType] > pieceValue[capturedType];
		}
	}

	for (int16_t i = 0; i < movesCnt; i++) {
		//Note: here we only guess how good a move is, so we can afford to make
		//guesses overinflated since the only thing that matters is their relative values
		const int16_t curMove = moves[i];
		const int8_t stPos = getStartPos(curMove), endPos = getEndPos(curMove);
		const PieceType pt = pos->m_pieceOnTile[stPos]->type;
		bool quiet = 1;//Quiet refers to moves that aren't capture, promotion or check
		int curGuess = 0, see;
		indices[i] = i;
		//Favor move in tt
		if (curMove == ttBestMove) {
			evalGuess[i] = 0x7FFF;//<- just a sufficiently large number
			continue;
		}
		see = pos->SEE(curMove);
		curGuess += see * 2;
		if (pt != PAWN && see <= -pieceValue[pt] + pieceValue[PAWN]) {
			curGuess -= pieceValue[pt] * 3 / 2;
		}

		//Add bonuses; Have to have if(black) here, because template doesn't accept it as argument
		if (pos->m_blackToMove) {
			curGuess += (getSqBonus<1>(pt, endPos) - getSqBonus<1>(pt, stPos)) * 8 / 5;
		} else {
			curGuess += (getSqBonus<0>(pt, endPos) - getSqBonus<0>(pt, stPos)) * 8 / 5;
		}

		//Add killers
		if (curMove == killers[movesHistory.size()][0] ||
			curMove == killers[movesHistory.size()][1]) {

			curGuess += KILLER_BONUS;
		}

		//If tile attacked by opponent pawn
		if ((1ULL << endPos) & pos->enemyPawnAttacksBitmask) {
			curGuess -= 4 * pieceValue[pt];
		}

		const int16_t* const k = killers[movesHistory.size() + 1];
		if ((k[0] != nullMove && endPos == getEndPos(k[0])) ||
			(k[1] != nullMove && endPos == getEndPos(k[1]))) {
			curGuess -= 8 * pieceValue[pt];
		}

		if (mateIfNullMove) {
			if ((1ULL << endPos) & (betweenBitboard[bmAfterNullEnd][bmAfterNullSt] |
				(pt == KING ? 0 : betweenBitboard[bmAfterNullEnd][friendlyKingPos]))) {
				curGuess += NULL_MOVE_MATE_DEFEND_BONUS / 2;
			}
			if (max(abs(getX(stPos) - getX(friendlyKingPos)), abs(getY(stPos) - getY(friendlyKingPos))) == 1) {
				curGuess += NULL_MOVE_MATE_DEFEND_BONUS / 42;
			}
			if (pt == KING) {
				curGuess += NULL_MOVE_MATE_DEFEND_BONUS / (21 - 7 * (abs(getX(stPos) - getX(endPos) == 2)));
			}
		}

		//Favor captures
		if (pos->isCapture(curMove)) {
			int8_t capturePos = endPos + (pos->isEnPassant(curMove) ? (pos->m_blackToMove ? 8 : -8) : 0);
			PieceType capturedType = pos->m_pieceOnTile[capturePos]->type;
			//13 * capturedPieceVal to prioritise captures before non-captures and 
			//to incentivise capturing more valuable pieces; We chosse 13, because
			//13 is the lowest number above how much pawns a queen is worth

			//pieceValue[KING] = inf; so king captures would be placed last; to prevent this add if (pt == KING)
			if (pt == KING) {
				curGuess += 13 * pieceValue[capturedType] - pieceValue[QUEEN] * 3 / 2;
			} else {
				curGuess += 13 * pieceValue[capturedType] - pieceValue[pt];
			}
			if (capturePos == bmAfterNullSt) {
				curGuess += NULL_MOVE_DEFEND_BONUS * 8;
			}
			curGuess += captureHistory[pos->m_blackToMove][pt][endPos][capturedType];
			if (see < -50) {
				curGuess += see * 2;
			}
			quiet = 0;
		}
		//Promoting is good
		if (getPromotionPiece(curMove) != UNDEF) {
			//Multiply by 15, because when capturing we multiply 
			//by 13 and promoting is more likely to be good (I think)
			curGuess += 15 * pieceValue[getPromotionPiece(curMove)];
			quiet = 0;
		}
		//Favor checks slightly
		if (givesCheck(curMove)) {
			//Add pieceValue[piece to make move] / 32, because generally checks with
			//queens and rooks are better (and there is a higher chance for a fork)
			curGuess += (CHECK_BONUS + (pieceValue[pt] / 32)) * (1 + mateIfNullMove);
			quiet = 0;
		}
		if (quiet) {
			curGuess -= pieceValue[PAWN];
			//If escaping from opponent threat
			if (stPos == bmAfterNullEnd) {
				if (nmIsBigCapture) {
					curGuess += pieceValue[pt] / 4;
				} else {
					curGuess += pieceValue[pt] / 8;
				}
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
			if (nullDefenders[pt] & (1ULL << endPos)) {
				if (mateIfNullMove) {
					curGuess += NULL_MOVE_MATE_DEFEND_BONUS;
				} else {
					if (!nmIsBigCapture && nmIsCapture) {
						const int x = nmScoreDelta;
						const int bonus = (floorLog2(x) * fastSqrt(x) / (floorLog2(x) + fastSqrt(x))) * x / 34;
						curGuess += bonus;
					} else {
						curGuess += floorLog2(nmScoreDelta - 16);
					}
				}
			}
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