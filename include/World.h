#pragma once
#include "SDL.h"
#include "Presenter.h"
#include "GraphicBoard.h"
#include "InputManager.h"

class World {
public:
	World();
	~World();

	void init();
	void update();
	void draw();

	static SDL_Texture* m_backgroundTexture;
	bool quit;
	GraphicBoard m_board;
private:
	Presenter m_presenter;
	InputManager m_input;
};