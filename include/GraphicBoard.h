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
	void initPos(Position* pos);
	void draw(int2 mouseCoords);
	void flip();

	void selectPiece(int2 mouseCoords);
	int16_t dropPiece(int2 mouseCoords);//Returns move if legal

	static SDL_Texture* m_possibleMoveTexture;
	static SDL_Texture* m_captureTexture;
private:
	int64_t flipAvalableTime;
	bool flipped;
	Drawable** m_tile;
	bool m_draggingSelectedPiece;
	int16_t* m_legalMoveTile;
	
	Position* m_pos;
	int8_t m_selectedPiecePos;
};