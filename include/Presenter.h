#pragma once 

#include <iomanip>
#include "Globals.h"
#include "Drawable.h"
#include "Piece.h"
using std::string;

class Presenter {
public:
    Presenter();
    ~Presenter();
    static SDL_Window* m_mainWindow;
    static SDL_Renderer* m_mainRenderer;

    static unsigned int m_SCREEN_WIDTH;
    static unsigned int m_SCREEN_HEIGHT;
    static SDL_Texture* m_BACKGROUND_TEXTURE;
    static SDL_Texture* m_PIECES;

    void init();
    void draw();

    SDL_Renderer* getRenderer();

    static void drawObject(SDL_Texture* texture);
    static void drawObject(Drawable& drawable);
    static void drawObject(SDL_Texture* texture, SDL_Rect rect);
    static void drawPiece(PieceType type, SDL_Rect& rect, bool black);
}; 