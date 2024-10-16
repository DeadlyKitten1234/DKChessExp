#include "GraphicBoard.h"
#include "Move.h"
#include "Clock.h"
#include "PrecomputedData.h"
#include "Presenter.h"
#include "Globals.h"
#include <iostream>
const int GraphicBoard::m_boardSizePx = 828;
SDL_Texture* GraphicBoard::m_possibleMoveTexture = nullptr;
SDL_Texture* GraphicBoard::m_captureTexture = nullptr;
SDL_Texture* GraphicBoard::m_nukeTexture = nullptr;

SDL_Texture* Nuke::tex = nullptr;
Nuke Nuke::nukes[8][8];
int64_t Nuke::stTime = 0;

GraphicBoard::GraphicBoard() {
	m_tile = nullptr;
	m_legalMoveTile = nullptr;
	m_SEE = nullptr;
	m_pos = nullptr;
	m_selectedPiecePos = -1;
	m_draggingSelectedPiece = 0;
	flipAvalableTime = 0;
	flipped = 0;
}

GraphicBoard::~GraphicBoard() {
	if (m_tile != nullptr) {
		for (int i = 0; i < 8; i++) {
			delete[] m_tile[i];
		}
	}
	if (m_legalMoveTile != nullptr) {
		delete[] m_legalMoveTile;
	}
	if (m_SEE != nullptr) {
		delete[] m_SEE;
	}
	if (m_pos != nullptr) {
		delete m_pos;
	}
}

void GraphicBoard::init() {
	SDL_Texture* WhiteTile = loadTexture("WhiteTile", Presenter::m_mainRenderer);
	SDL_Texture* BlackTile = loadTexture("BlackTile", Presenter::m_mainRenderer);
	m_possibleMoveTexture = loadTexture("PossibleMove", Presenter::m_mainRenderer);
	m_captureTexture = loadTexture("CaptureMove", Presenter::m_mainRenderer);
	Nuke::init();
	SDL_SetTextureAlphaMod(m_possibleMoveTexture, 50);
	SDL_SetTextureColorMod(m_possibleMoveTexture, 0, 0, 0);
	m_legalMoveTile = new int16_t[64]();
	m_SEE = new int16_t[64]();
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

void GraphicBoard::initPos(Position* pos) {
	m_pos = pos;
}

void GraphicBoard::draw(int2 mouseCoords) {
	//Draw tiles
	for (int i = 0; i < 8; i++) {
		for (int j = 0; j < 8; j++) {
			Drawable* drawTile = &m_tile[i][j];
			if (flipped) {
				drawTile = &m_tile[7 - i][7 - j];
			}
			Presenter::drawObject(*drawTile);
			if (m_legalMoveTile[(i * 8) + j] != 0) {
				//Capture or en passant
				if (m_pos->m_pieceOnTile[(i * 8) + j] != nullptr || 
					(m_pos->m_pieceOnTile[m_selectedPiecePos]->type == PAWN && i * 8 + j == m_pos->m_possibleEnPassant)) {
					if (givesCheck(m_legalMoveTile[i * 8 + j])) {
						SDL_SetTextureColorMod(m_captureTexture, m_SEE[i * 8 + j], 255, 0);
					}
					Presenter::drawObject(m_captureTexture, drawTile->m_rect);
					if (givesCheck(m_legalMoveTile[i * 8 + j])) {
						SDL_SetTextureColorMod(m_captureTexture, 255, 255, 255);
					}
				} else {
					if (givesCheck(m_legalMoveTile[i * 8 + j])) {
						SDL_SetTextureColorMod(m_captureTexture, 0, 255, 0);
						SDL_SetTextureAlphaMod(m_captureTexture, 180);
						Presenter::drawObject(m_captureTexture, drawTile->m_rect);
						SDL_SetTextureColorMod(m_captureTexture, 255, 255, 255);
						SDL_SetTextureAlphaMod(m_captureTexture, 255);
					} else {
						Presenter::drawObject(m_possibleMoveTexture, drawTile->m_rect);
					}
				}
			}
		}
	}
	//Draw pieces
	int tileSz = m_boardSizePx / 8;
	for (int i = 0; i < m_pos->m_whiteTotalPiecesCnt; i++) {
		SDL_Rect pieceRect = { 0, 0, tileSz, tileSz };
		if (m_selectedPiecePos == m_pos->m_whitePiece[i]->pos && m_draggingSelectedPiece) {
			continue;
		}
		pieceRect.x =	(Presenter::m_SCREEN_WIDTH - m_boardSizePx) / 2 + 
						tileSz * (flipped ? 7 - m_pos->m_whitePiece[i]->x : m_pos->m_whitePiece[i]->x);

		pieceRect.y =	(Presenter::m_SCREEN_HEIGHT + m_boardSizePx) / 2 - 
						tileSz * (flipped ? 7 - m_pos->m_whitePiece[i]->y + 1 : m_pos->m_whitePiece[i]->y + 1);
		Presenter::drawPiece(m_pos->m_whitePiece[i]->type, pieceRect, m_pos->m_whitePiece[i]->black);
	}
	for (int i = 0; i < m_pos->m_blackTotalPiecesCnt; i++) {
		SDL_Rect pieceRect = { 0, 0, tileSz, tileSz };
		if (m_selectedPiecePos == m_pos->m_blackPiece[i]->pos && m_draggingSelectedPiece) {
			continue;
		}
		pieceRect.x =	(Presenter::m_SCREEN_WIDTH - m_boardSizePx) / 2 + 
						tileSz * (flipped ? 7 - m_pos->m_blackPiece[i]->x : m_pos->m_blackPiece[i]->x);
		pieceRect.y =	(Presenter::m_SCREEN_HEIGHT + m_boardSizePx) / 2 - 
						tileSz * (flipped ? 7 - m_pos->m_blackPiece[i]->y + 1 : m_pos->m_blackPiece[i]->y + 1);
		Presenter::drawPiece(m_pos->m_blackPiece[i]->type, pieceRect, m_pos->m_blackPiece[i]->black);
	}
	//Draw selected piece
	if (m_selectedPiecePos != -1 && m_draggingSelectedPiece) {
		const Piece* selPiece = m_pos->m_pieceOnTile[m_selectedPiecePos];
		SDL_Rect pieceRect = { int(mouseCoords.x - tileSz * 0.7), int(mouseCoords.y - tileSz * 0.7), int(tileSz * 1.4), int(tileSz * 1.4) };
		Presenter::drawPiece(selPiece->type, pieceRect, selPiece->black);
	}
	//Draw nukes (why?)
	for (int i = 0; i < 8; i++) {
		for (int j = 0; j < 8; j++) {
			Drawable* drawTile = &m_tile[i][j];
			if (flipped) {
				drawTile = &m_tile[7 - i][7 - j];
			}
			Nuke* drawNuke = &Nuke::nukes[i][j];
			//drawNuke->activate();
			drawNuke->render(drawTile);
			if (drawNuke->active) {
				if (Clock::now() > Nuke::stTime + 50) {
					drawNuke->incrY();
				}
			}
		}
	}
	if (Clock::now() > Nuke::stTime + 50) {
		Nuke::stTime = Clock::now();
	}
}

void GraphicBoard::flip(bool ignoreDelay) {
	if (ignoreDelay || SDL_GetTicks64() > flipAvalableTime) {
		flipped = !flipped;
		flipAvalableTime = SDL_GetTicks64() + 250;
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
			if (flipped) {
				tileX = 7 - tileX;
				tileY = 7 - tileY;
			}
			//Piece on tile exists and is the correct color
			if (m_pos->m_pieceOnTile[tileX + 8 * tileY] != nullptr && m_pos->m_pieceOnTile[tileX + 8 * tileY]->black == m_pos->m_blackToMove) {
				m_selectedPiecePos = tileX + 8 * tileY;
				m_draggingSelectedPiece = 1;

				for (int i = 0; i < 64; i++) {
					m_legalMoveTile[i] = 0;
					m_SEE[i] = 0;
				}
				for (int i = 0; i < m_pos->m_legalMovesCnt; i++) {
					if (getStartPos(m_pos->m_legalMoves[i]) == m_selectedPiecePos) {
						m_legalMoveTile[getEndPos(m_pos->m_legalMoves[i])] = m_pos->m_legalMoves[i];
						m_SEE[getEndPos(m_pos->m_legalMoves[i])] = m_pos->SEE(m_pos->m_legalMoves[i]);
					}
				}
				//for (int i = 0; i < 64; i++) {
				//	cout << setw(4) << m_SEE[i];
				//	if (i % 8 == 7) { cout << '\n'; }
				//}
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
			if (flipped) {
				tileX = 7 - tileX;
				tileY = 7 - tileY;
			}
			if (m_legalMoveTile[tileX + 8 * tileY]) {
				int16_t ans = createMove(m_selectedPiecePos, tileX + 8 * tileY, 0);
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
