#pragma once
#include "InputManager.h"
#include "GraphicBoard.h"
#include "AI.h"

class GameManager {
public:
	GameManager();
	~GameManager();

	void update(const InputManager& input);
	void init(Position* pos_ = nullptr);
	void initPos(Position* pos_);
	void setAI(bool whiteIsAI, bool blackIsAI) { playerAI[0] = whiteIsAI; playerAI[1] = blackIsAI; }

	bool playerAI[2];

	GraphicBoard m_board;
	Position* m_pos;
	AI m_ai;
};