#include "GraphicBoard.h"
#include "Move.h"
#include "PrecomputedData.h"
#include "Presenter.h"
#include "Globals.h"
#include <iostream>
const int GraphicBoard::m_boardSizePx = 828;
SDL_Texture* GraphicBoard::m_possibleMoveTexture = nullptr;
SDL_Texture* GraphicBoard::m_captureTexture = nullptr;

GraphicBoard::GraphicBoard() {
	m_tile = nullptr;
	m_legalMoveTile = nullptr;
	m_pos = nullptr;
	m_selectedPiecePos = -1;
	m_draggingSelectedPiece = 0;
}

GraphicBoard::~GraphicBoard() {}

void GraphicBoard::init() {
	SDL_Texture* WhiteTile = loadTexture("WhiteTile", Presenter::m_mainRenderer);
	SDL_Texture* BlackTile = loadTexture("BlackTile", Presenter::m_mainRenderer);
	m_possibleMoveTexture = loadTexture("PossibleMove", Presenter::m_mainRenderer);
	m_captureTexture = loadTexture("CaptureMove", Presenter::m_mainRenderer);
	m_legalMoveTile = new int16_t[64]();
	m_tile = new Drawable*[8]();
	int tileSz = m_boardSizePx / 8;
	int x = (Presenter::m_SCREEN_WIDTH - m_boardSizePx) / 2;
	int y = (Presenter::m_SCREEN_HEIGHT + m_boardSizePx) / 2 - tileSz;
	for (int i = 0; i < 8; i++) {
		m_tile[i] = new Drawable[8]();
		for (int j = 0; j < 8; j++) {
			m_tile[i][j] = Drawable();
			m_tile[i][j].m_rect = { x, y, tileSz, tileSz };
			if (((i + j) & 1) == 0) {
				m_tile[i][j].m_texture = BlackTile;
			} else {
				m_tile[i][j].m_texture = WhiteTile;
			}
			x += tileSz;
		}
		y -= tileSz;
		x -= 8 * tileSz;
	}
}

void GraphicBoard::initPos(Position*  pos) {
	m_pos = pos;
}

void GraphicBoard::draw(int2 mouseCoords) {
	for (int i = 0; i < 8; i++) {
		for (int j = 0; j < 8; j++) {
			Presenter::drawObject(m_tile[i][j]);
			if (m_legalMoveTile[(i << 3) + j] != 0) {
				//Capture or en passant
				if (m_pos->m_pieceOnTile[(i << 3) + j] != nullptr || 
					(m_pos->m_pieceOnTile[m_selectedPiecePos]->type == PAWN && i * 8 + j == m_pos->m_possibleEnPassant)) {
					
					Presenter::drawObject(m_captureTexture, m_tile[i][j].m_rect);
				} else {
					Presenter::drawObject(m_possibleMoveTexture, m_tile[i][j].m_rect);
				}
			}
		}
	}
	int tileSz = m_boardSizePx / 8;
	for (int i = 0; i < m_pos->m_whiteTotalPiecesCnt; i++) {
		SDL_Rect pieceRect = { 0, 0, tileSz, tileSz };
		if (m_selectedPiecePos == m_pos->m_whitePiece[i]->pos && m_draggingSelectedPiece) {
			continue;
		}
		pieceRect.x = (Presenter::m_SCREEN_WIDTH - m_boardSizePx) / 2 + tileSz * m_pos->m_whitePiece[i]->x;
		pieceRect.y = (Presenter::m_SCREEN_HEIGHT + m_boardSizePx) / 2 - tileSz * (m_pos->m_whitePiece[i]->y + 1);
		Presenter::drawPiece(m_pos->m_whitePiece[i]->type, pieceRect, m_pos->m_whitePiece[i]->black);
	}
	for (int i = 0; i < m_pos->m_blackTotalPiecesCnt; i++) {
		SDL_Rect pieceRect = { 0, 0, tileSz, tileSz };
		if (m_selectedPiecePos == m_pos->m_blackPiece[i]->pos && m_draggingSelectedPiece) {
			continue;
		}
		pieceRect.x = (Presenter::m_SCREEN_WIDTH - m_boardSizePx) / 2 + tileSz * m_pos->m_blackPiece[i]->x;
		pieceRect.y = (Presenter::m_SCREEN_HEIGHT + m_boardSizePx) / 2 - tileSz * (m_pos->m_blackPiece[i]->y + 1);
		Presenter::drawPiece(m_pos->m_blackPiece[i]->type, pieceRect, m_pos->m_blackPiece[i]->black);
	}
	if (m_selectedPiecePos != -1 && m_draggingSelectedPiece) {
		const Piece* selPiece = m_pos->m_pieceOnTile[m_selectedPiecePos];
		SDL_Rect pieceRect = { int(mouseCoords.x - tileSz * 0.7), int(mouseCoords.y - tileSz * 0.7), int(tileSz * 1.4), int(tileSz * 1.4) };
		Presenter::drawPiece(selPiece->type, pieceRect, selPiece->black);
	}
}

void GraphicBoard::selectPiece(int2 mouseCoords) {
	int tileSz = m_boardSizePx / 8;
	int boardStX = (Presenter::m_SCREEN_WIDTH - m_boardSizePx) / 2;
	int boardStY = (Presenter::m_SCREEN_HEIGHT + m_boardSizePx) / 2 - tileSz;
	//Click in board
	if (boardStX <= mouseCoords.x && mouseCoords.x <= boardStX + m_boardSizePx) {
		if (boardStY + tileSz >= mouseCoords.y && mouseCoords.y >= boardStY + tileSz - m_boardSizePx) {
			int tileX = (mouseCoords.x - boardStX) / tileSz;
			int tileY = (boardStY + tileSz - mouseCoords.y) / tileSz;
			//Piece on tile exists and is the correct color
			if (m_pos->m_pieceOnTile[tileX + 8 * tileY] != nullptr && m_pos->m_pieceOnTile[tileX + 8 * tileY]->black == m_pos->m_blackToMove) {
				m_selectedPiecePos = tileX + 8 * tileY;
				m_draggingSelectedPiece = 1;

				for (int i = 0; i < 64; i++) {
					m_legalMoveTile[i] = 0;
				}
				for (int i = 0; i < m_pos->m_legalMovesCnt; i++) {
					if (getStartPos(m_pos->m_legalMoves[i]) == m_selectedPiecePos) {
						m_legalMoveTile[getEndPos(m_pos->m_legalMoves[i])] = m_pos->m_legalMoves[i];
					}
				}
			}
		}
	}
}

int16_t GraphicBoard::dropPiece(int2 mouseCoords) {
	m_draggingSelectedPiece = 0;
	int tileSz = m_boardSizePx / 8;
	int boardStX = (Presenter::m_SCREEN_WIDTH - m_boardSizePx) / 2;
	int boardStY = (Presenter::m_SCREEN_HEIGHT + m_boardSizePx) / 2 - tileSz;
	if (boardStX <= mouseCoords.x && mouseCoords.x <= boardStX + m_boardSizePx) {
		if (boardStY + tileSz >= mouseCoords.y && mouseCoords.y >= boardStY + tileSz - m_boardSizePx) {
			int tileX = (mouseCoords.x - boardStX) / tileSz;
			int tileY = (boardStY + tileSz - mouseCoords.y) / tileSz;
			if (m_legalMoveTile[tileX + 8 * tileY]) {
				int16_t ans = createMove(m_selectedPiecePos, tileX + 8 * tileY);
				//Reset selection and update legal moves
				m_selectedPiecePos = -1;
				for (int i = 0; i < 64; i++) {
					m_legalMoveTile[i] = 0;
				}
				return ans;
			}
		}
	}
	return nullMove;
}
