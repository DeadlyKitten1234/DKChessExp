#include "Globals.h"

float MULTIPLY_X = 1;
float MULTIPLY_Y = 1;
SDL_DisplayMode SCREEN_RESOLUTION;

SDL_Texture* loadTexture(string in, SDL_Renderer* renderer) {
	in = "assets\\img\\" + in + ".bmp";
	SDL_Surface* loadingSurface = SDL_LoadBMP(in.c_str());
	SDL_Texture* out = nullptr;
	out = SDL_CreateTextureFromSurface(renderer, loadingSurface);
	SDL_FreeSurface(loadingSurface);
	return out;
}

bool coordsInRect(int x, int y, SDL_Rect rect) {
	if ((rect.x <= x && x <= rect.x + rect.w) && (rect.y <= y && y <= rect.y + rect.h)) {
		return true;
	}
	return false;
}

int2::int2() { x = -1; y = -1; }

int2::int2(int st_x, int st_y) {
	x = st_x;
	y = st_y;
}
