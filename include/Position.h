#pragma once
#include "Piece.h"
class Position {
public:
	Position();
	~Position();

	void readFEN(const char* fen);

	int8_t makeMove(int16_t move);//Returns captured piece idx
	void undoMove(int16_t move, int8_t capturedPieceIdx, int8_t bitmaskCastling, int8_t possibleEnPassant);
	void updateLegalMoves();
	int16_t evaluate();
	
	static void initLegalMoves();
	static const char* m_startFEN;
	static int16_t* m_legalMoves;

	int m_legalMovesStartIdx;

	int16_t m_legalMovesCnt;

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

private:
	void deleteData();
};