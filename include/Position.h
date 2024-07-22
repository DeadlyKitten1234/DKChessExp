#pragma once
#include "Piece.h"
#include <algorithm>//min and max
using std::abs;
using std::min;
using std::max;
class Position {
public:
	Position();
	~Position();

	void readFEN(const char* fen);

	int8_t makeMove(int16_t move);//Returns captured piece idx
	void undoMove(int16_t move, int8_t capturedPieceIdx, int8_t bitmaskCastling, int8_t possibleEnPassant);
	template<bool capturesOnly>
	void updateLegalMoves();
	inline int16_t evaluate() {
		int16_t eval = m_whitePiecesEval - m_blackPiecesEval;
		eval += forceKingToEdgeEval<0>();
		eval -= forceKingToEdgeEval<1>();
		return eval * (m_blackToMove ? -1 : 1);
	}

	static void initLegalMoves();
	static const char* m_startFEN;

	static int16_t* m_legalMoves;
	int16_t m_legalMovesStartIdx;
	int16_t m_legalMovesCnt;

	int16_t m_whitePiecesEval;
	int16_t m_blackPiecesEval;

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
	const int16_t endgameMaterialStart = pieceValue[ROOK] + pieceValue[QUEEN] + pieceValue[BISHOP] * 2;
	//Return range [0, 100]
	inline int16_t getEndgameWeight() {
		//Implementation of this function taken from Sebastian Lague
		//https://github.com/SebLague/Chess-Coding-Adventure/blob/Chess-V1-Unity/Assets/Scripts/Core/AI/Evaluation.cs
		const int16_t pcsValueWithoutPawns = m_whitePiecesEval + m_blackPiecesEval - (m_whitePiecesCnt[PAWN] + m_blackPiecesCnt[PAWN]) * pieceValue[PAWN];
		//return = 128 * (1 - (pcsValueWithoutPawns / endgameMaterialStart))
		return max(0, ((endgameMaterialStart - pcsValueWithoutPawns) << 7) / endgameMaterialStart);
	}
	template<bool blackPerspective>
	int16_t forceKingToEdgeEval();
	void deleteData();
};

#include "MoveGenerator.h"//Include hacks so doesn't CE. do not move to beggining of file

template<bool capturesOnly>
void Position::updateLegalMoves() {
	m_legalMovesCnt = generateMoves<capturesOnly>(*this, m_legalMoves, m_legalMovesStartIdx);
	friendlyInCheck = inCheck;//inCheck is declared in MoveGenerator.h
	enemyPawnAttacksBitmask = pawnAtt;//pawnAtt declared in MoveGenerator.h
}

template<bool blackPerspective>
inline int16_t Position::forceKingToEdgeEval() {
	//Idea and implementation of this function and getEndgameWeight() taken from Sebastian Lague
	//https://github.com/SebLague/Chess-Coding-Adventure/blob/Chess-V1-Unity/Assets/Scripts/Core/AI/Evaluation.cs
	int16_t endgameWeight = getEndgameWeight();
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
	//Expression below is 4 * (14 - manhatan distance between kings)
	eval += (14 - abs(friendlyKing->x - enemyKing->x) - abs(friendlyKing->y - enemyKing->y)) * 4;
	return (eval * endgameWeight) >> 6;
}