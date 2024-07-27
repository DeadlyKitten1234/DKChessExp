#pragma once
#include "Piece.h"
#include "EvalValues.h"
#include <algorithm>//min
using std::abs;
using std::min;
class Position {
public:
	Position();
	~Position();

	void readFEN(const char* fen);

	template<bool black>
	//stTile == -1 means revert capture; endTile == -1 means make capture
	inline void updateDynamicVars(const PieceType type, const int8_t stTile, int8_t endTile);
	uint64_t getZHashIfMoveMade(const int16_t move) const;
	int8_t makeMove(int16_t move);//Returns captured piece idx
	void undoMove(int16_t move, int8_t capturedPieceIdx, int8_t bitmaskCastling_, int8_t possibleEnPassant_);
	template<bool capturesOnly>
	void updateLegalMoves();
	inline int16_t evaluate();

	static void initLegalMoves();
	static const char* m_startFEN;

	static int16_t* m_legalMoves;
	int16_t m_legalMovesStartIdx;
	int16_t m_legalMovesCnt;

	int16_t whiteEndgameWeight;//If = -1 means needs an update
	int16_t blackEndgameWeight;//If = -1 means needs an update

	int16_t m_whitePiecesEval;
	int16_t m_blackPiecesEval;
	int16_t m_whiteSqBonusEval;
	int16_t m_blackSqBonusEval;

	uint64_t zHash;//zobrist hash number
	//Pieces
	Piece** m_whitePiece;
	Piece** m_blackPiece;
	int8_t m_whiteTotalPiecesCnt;
	int8_t m_blackTotalPiecesCnt;
	int8_t* m_whitePiecesCnt;
	int8_t* m_blackPiecesCnt;
	uint64_t m_whiteAllPiecesBitboard;
	uint64_t m_blackAllPiecesBitboard;
	uint64_t* m_whiteBitboards;
	uint64_t* m_blackBitboards;

	Piece** m_pieceOnTile;

	bool m_blackToMove;
	int8_t m_bitmaskCastling;//First white; First king-side
	int8_t m_possibleEnPassant;

	bool friendlyInCheck;//Call updateLegalMoves before trusting
	uint64_t enemyPawnAttacksBitmask;//Call updateLegalMoves before trusting

private:
	template<bool blackPerspective>
	inline int16_t forceKingToEdgeEval();
	void deleteData();
};

#include "MoveGenerator.h"//Include hacks so doesn't CE. do not move to beggining of file

template<bool black>
inline void Position::updateDynamicVars(const PieceType type, const int8_t stTile, int8_t endTile) {
	//If stTile == -1 { revert capture; don't update start }
	//If endTile == -1 { make capture; don't update end }
	//Which makes sense, stTile = -1 means piece "appeared from nowhere"
						//endTile = -1 means piece "disappeared"
	if constexpr (black) {
		if (stTile != -1) {
			m_blackAllPiecesBitboard &= ~(1ULL << stTile);
			m_blackBitboards[type] &= ~(1ULL << stTile);
			m_blackSqBonusEval -= getSqBonus<1>(type, stTile);
		} else {
			//Revert capture
			m_blackTotalPiecesCnt++;
			m_blackPiecesCnt[type]++;
			m_blackPiecesEval += pieceValue[type];
		}
		if (endTile != -1) {
			m_blackAllPiecesBitboard |= (1ULL << endTile);
			m_blackBitboards[type] |= (1ULL << endTile);
			m_blackSqBonusEval += getSqBonus<1>(type, endTile);
		} else {
			//Make capture
			m_blackTotalPiecesCnt--;
			m_blackPiecesCnt[type]--;
			m_blackPiecesEval -= pieceValue[type];
		}
	} else {
		if (stTile != -1) {
			m_whiteAllPiecesBitboard &= ~(1ULL << stTile);
			m_whiteBitboards[type] &= ~(1ULL << stTile);
			m_whiteSqBonusEval -= getSqBonus<0>(type, stTile);
		} else {
			//Revert capture
			m_whiteTotalPiecesCnt++;
			m_whitePiecesCnt[type]++;
			m_whitePiecesEval += pieceValue[type];
		}
		if (endTile != -1) {
			m_whiteAllPiecesBitboard |= (1ULL << endTile);
			m_whiteBitboards[type] |= (1ULL << endTile);
			m_whiteSqBonusEval += getSqBonus<0>(type, endTile);
		} else {
			//Make capture
			m_whiteTotalPiecesCnt--;
			m_whitePiecesCnt[type]--;
			m_whitePiecesEval -= pieceValue[type];
		}
	}
	if (stTile != -1) {
		zHash ^= hashNums[stTile][black][type];
	}
	if (endTile != -1) {
		zHash ^= hashNums[endTile][black][type];
	}
}

template<bool capturesOnly>
void Position::updateLegalMoves() {
	m_legalMovesCnt = generateMoves<capturesOnly>(*this, m_legalMoves, m_legalMovesStartIdx);
	friendlyInCheck = inCheck;//inCheck is declared in MoveGenerator.h
	enemyPawnAttacksBitmask = pawnAtt;//pawnAtt declared in MoveGenerator.h
}

inline int16_t Position::evaluate() {
	//Add pieces
	int16_t eval = m_whitePiecesEval - m_blackPiecesEval;
	//Add sq bonuses
	eval += m_whiteSqBonusEval - m_blackSqBonusEval;
	//If endgame eval is marked as 'needs update', update it
	if (whiteEndgameWeight == -1) {
		whiteEndgameWeight = getEndgameWeight(m_whitePiecesEval, m_whitePiecesCnt[PAWN]);
	}
	if (blackEndgameWeight == -1) {
		blackEndgameWeight = getEndgameWeight(m_blackPiecesEval, m_blackPiecesCnt[PAWN]);
	}
	//Change king bonuses according to endgame weight; If is there to save time
	if (whiteEndgameWeight != 0) {
		eval -= getSqBonus<0>(KING, m_whitePiece[0]->pos);//Remove old bonus
		eval += getKingSqBonus<0>(m_whitePiece[0]->pos, whiteEndgameWeight);//Add new bonus
	}
	if (blackEndgameWeight != 0) {
		//Signs are flipped, because black
		eval += getSqBonus<1>(KING, m_blackPiece[0]->pos);//Remove old bonus
		eval -= getKingSqBonus<1>(m_blackPiece[0]->pos, blackEndgameWeight);//Add new bonus
	}
	//Force king to edge in the endgame
	eval += forceKingToEdgeEval<0>() - forceKingToEdgeEval<1>();
	return eval * (m_blackToMove ? -1 : 1);

}

template<bool blackPerspective>
inline int16_t Position::forceKingToEdgeEval() {
	//Idea and implementation of this function and getEndgameWeight() taken from Sebastian Lague
	//https://github.com/SebLague/Chess-Coding-Adventure/blob/Chess-V1-Unity/Assets/Scripts/Core/AI/Evaluation.cs
	int16_t friendlyEvalNoPawns;
	if constexpr (!blackPerspective) {
		friendlyEvalNoPawns = m_whitePiecesEval - m_whitePiecesCnt[PAWN] * pieceValue[PAWN];
	} else {
		friendlyEvalNoPawns = m_blackPiecesEval - m_blackPiecesCnt[PAWN] * pieceValue[PAWN];
	}

	int16_t endgameWeight = getEndgameWeight(friendlyEvalNoPawns);
	bool worthEvaluating = 0;
	if constexpr (blackPerspective) {
		worthEvaluating = m_blackPiecesEval >= (m_whitePiecesEval + pieceValue[PAWN] * 2);
	} else {
		worthEvaluating = m_whitePiecesEval >= (m_blackPiecesEval + pieceValue[PAWN] * 2);
	}
	if (!(endgameWeight > 1 && worthEvaluating)) {
		return 0;
	}
	Piece* friendlyKing = nullptr;
	Piece* enemyKing = nullptr;
	if constexpr (blackPerspective) {
		friendlyKing = m_blackPiece[0];
		enemyKing = m_whitePiece[0];
	} else {
		friendlyKing = m_whitePiece[0];
		enemyKing = m_blackPiece[0];
	}
	int8_t enemyKingDistFromCenter = 6 - min<int8_t>(enemyKing->x, 7 - enemyKing->x) - min<int8_t>(enemyKing->y, 7 - enemyKing->y);
	int16_t eval = enemyKingDistFromCenter * 10;
	//Expression below is (14 - manhatan distance between kings) * weight
	eval += (14 - abs(friendlyKing->x - enemyKing->x) - abs(friendlyKing->y - enemyKing->y)) * 6;
	return (eval * endgameWeight) >> 6;
}