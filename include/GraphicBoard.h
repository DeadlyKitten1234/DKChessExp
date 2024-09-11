#pragma once

#include "Drawable.h"
#include "Globals.h"
#include "Position.h"
#include "Piece.h"
#include "Presenter.h"

class Nuke {
public:
	Nuke() : srcRect({ 0, 0, TEX_SIZE, TEX_SIZE }), active(false) {  }
	~Nuke() {  }
	
	SDL_Rect srcRect;
	bool active;

	void activate() { srcRect.y = 0; active = true; }
	void incrY() { srcRect.y += TEX_SIZE; if (srcRect.y >= 8 * TEX_SIZE) active = false; }
	void render(Drawable* tile) {
		if (!active) { return; }
		SDL_Rect dstRect = tile->m_rect;
		dstRect.x += dstRect.w / 2 - dstRect.w * 0.625;
		dstRect.y += dstRect.h / 2 - dstRect.h * 0.625;
		dstRect.w *= 1.25;
		dstRect.h *= 1.25;
		SDL_RenderCopy(Presenter::m_mainRenderer, tex, &srcRect, &dstRect);
	}

	static void init() { tex = loadTexture("nuke", Presenter::m_mainRenderer); }

	static SDL_Texture* tex;
	static const int TEX_SIZE = 128;

	static int64_t stTime;

	static Nuke nukes[8][8];
};


class GraphicBoard {
public:
	GraphicBoard();
	~GraphicBoard();

	const static int m_boardSizePx;

	void init();
	void initPos(Position* pos);
	void draw(int2 mouseCoords);
	void flip(bool ignoreDelay = false);

	void selectPiece(int2 mouseCoords);
	int16_t dropPiece(int2 mouseCoords);//Returns move if legal

	static SDL_Texture* m_possibleMoveTexture;
	static SDL_Texture* m_captureTexture;
	static SDL_Texture* m_nukeTexture;

private:
	int64_t flipAvalableTime;
	bool flipped;
	Drawable** m_tile;
	bool m_draggingSelectedPiece;
	int16_t* m_legalMoveTile;
	int16_t* m_SEE;
	
	Position* m_pos;
	int8_t m_selectedPiecePos;
};