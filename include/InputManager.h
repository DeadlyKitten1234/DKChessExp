#pragma once

#include "SDL.h"
#include "Globals.h"

class InputManager {
public:
	InputManager();
	~InputManager();

	int2 m_mouseCoord;
	bool m_mouseDown;
	bool m_mouseHeld;
	bool m_mouseUp;
	bool m_mouseMoved;

	bool m_keyReleased;
	bool m_quit;

	char keyPressed;//WORKS ONLY IN THE CHESS BOT; DO NOT COPY

	void getInput();
};