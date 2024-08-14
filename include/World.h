#pragma once
#include "SDL.h"
#include "Presenter.h"
#include "GameManager.h"
#include "InputManager.h"

class World {
public:
	World();
	~World();

	void init();
	void initPos(Position* pos_) { m_gameManager.initPos(pos_); pos_->updateLegalMoves<0>(true); };
	void update();
	void draw();

	static SDL_Texture* m_backgroundTexture;
	bool quit;

private:
	Presenter m_presenter;
	InputManager m_input;
	GameManager m_gameManager;
};