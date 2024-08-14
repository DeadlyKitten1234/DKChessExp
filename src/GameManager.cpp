#include "GameManager.h"

GameManager::GameManager() { m_pos = nullptr; playerAI[0] = playerAI[1] = 0; }

GameManager::~GameManager() {
	if (m_pos != nullptr) {
		delete m_pos;
	}
}

void GameManager::update(const InputManager& input) {
	if (playerAI[m_pos->m_blackToMove]) {
		m_ai.startSearch(1200);
		m_pos->makeMove(m_ai.bestMove);
		m_pos->updateLegalMoves<0>(true);
		return;
	}

	if (input.m_mouseDown) {
		m_board.selectPiece(input.m_mouseCoord);
	} else {
		if (input.m_mouseUp) {
			int16_t res = m_board.dropPiece(input.m_mouseCoord);
			PieceType promotionType = getTypeFromChar(input.keyPressed);
			if (promotionType == PAWN || promotionType == KING) {
				promotionType = UNDEF;
			}
			if (res != nullMove) {
				if (m_pos->m_pieceOnTile[getStartPos(res)]->type == PieceType::PAWN) {
					if (getY(getEndPos(res)) == (m_pos->m_blackToMove ? 0 : 7)) {
						res = createMove(getStartPos(res), getEndPos(res), 0, (promotionType == UNDEF ? QUEEN : promotionType));
					}
				}
				m_pos->makeMove(res);
				m_pos->updateLegalMoves<0>(true);
			}
		}
	}
	if (input.keyPressed == 'f') {
		m_board.flip();
	}
}

void GameManager::init(Position* pos_) {
	m_board.init();
	if (pos_ == nullptr) {
		m_pos = new Position();
		m_pos->readFEN(Position::m_startFEN);
	} else {
		m_pos = pos_;
	}
	m_board.initPos(pos_);
	m_ai.initPos(pos_);
}

void GameManager::initPos(Position* pos_) {
	m_pos = pos_;
	m_board.initPos(pos_);
	m_ai.initPos(pos_);
}
