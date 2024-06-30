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
	m_board.init();
}

void World::update() {
	m_input.getInput();
	if (m_input.m_mouseDown) {
		m_board.selectPiece(m_input.m_mouseCoord);
		//generateMoves(m_board.m_pos);
	}
	if (m_input.m_mouseUp) {
		m_board.dropPiece(m_input.m_mouseCoord, getTypeFromChar(m_input.keyPressed));
	}
	if (m_input.m_quit) {
		quit = 1;
		return;
	}
}

void World::draw() {
	Presenter::drawObject(m_backgroundTexture);
	m_board.draw(m_input.m_mouseCoord);
	m_presenter.draw();
}
