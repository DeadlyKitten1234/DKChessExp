#include <iostream>
#include "World.h"
#include "PrecomputedData.h"
#include "Move.h"
using std::cout;

SDL_Texture* World::m_backgroundTexture = nullptr;

World::World() {
	quit = 0;
}

World::~World() {}

void World::init() {
	m_presenter.init();
	m_backgroundTexture = loadTexture("Background", Presenter::m_mainRenderer);
	m_gameManager.init();

	m_gameManager.setAI(0, 1);
}

void World::update() {
	m_input.getInput();
	m_gameManager.update(m_input);
	if (m_input.m_quit) {
		quit = 1;
		return;
	}
}

void World::draw() {
	Presenter::drawObject(m_backgroundTexture);
	m_gameManager.m_board.draw(m_input.m_mouseCoord);
	m_presenter.draw();
}
