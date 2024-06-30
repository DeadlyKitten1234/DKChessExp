#pragma once

#include "Drawable.h"
#include "Globals.h"
#include "Position.h"
#include "Piece.h"

class GraphicBoard {
public:
	GraphicBoard();
	~GraphicBoard();

	const static int m_boardSizePx;

	void init();
	void draw(int2 mouseCoords);

	void selectPiece(int2 mouseCoords);
	void dropPiece(int2 mouseCoords, PieceType promotionType = PieceType::UNDEF);

	static SDL_Texture* m_possibleMoveTexture;
	static SDL_Texture* m_captureTexture;
	
	Drawable** m_tile;
	bool m_draggingSelectedPiece;
	int16_t* m_legalMoveTile;
	
	Position m_pos;
	int8_t m_selectedPiecePos;
};