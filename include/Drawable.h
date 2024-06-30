#pragma once
#include "SDL.h"

class Drawable {
public:
	Drawable();
	~Drawable();

	SDL_Rect m_rect;
	SDL_Texture* m_texture;
	int m_alpha;
};