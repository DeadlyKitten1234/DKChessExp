#pragma once

#include<iostream>
#include "SDL.h"
using namespace std;

extern SDL_DisplayMode SCREEN_RESOLUTION;

SDL_Texture* loadTexture(string in, SDL_Renderer* renderer);

struct int2 {
	int2();
	int2(int st_x, int st_y);
	int x;
	int y;
};

bool coordsInRect(int x, int y, SDL_Rect rect);