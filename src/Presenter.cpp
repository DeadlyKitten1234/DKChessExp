#include "Presenter.h"

SDL_Texture* Presenter::m_BACKGROUND_TEXTURE = nullptr;
SDL_Texture* Presenter::m_PIECES = nullptr;
SDL_Window* Presenter::m_mainWindow = nullptr;
SDL_Renderer* Presenter::m_mainRenderer = nullptr;
unsigned int Presenter::m_SCREEN_WIDTH = 0;
unsigned int Presenter::m_SCREEN_HEIGHT = 0;

SDL_Renderer* Presenter::getRenderer() {
    return m_mainRenderer;
}

Presenter::Presenter() {}

Presenter::~Presenter() {}

void Presenter::init() {
    m_SCREEN_WIDTH = 1536;
    m_SCREEN_HEIGHT = 864;

    SDL_Init(SDL_INIT_EVERYTHING);
    m_mainWindow = SDL_CreateWindow("ChessAI", 0, 0, m_SCREEN_WIDTH, m_SCREEN_HEIGHT, 0);
    m_mainRenderer = SDL_CreateRenderer(m_mainWindow, -1, SDL_RENDERER_ACCELERATED);
    m_BACKGROUND_TEXTURE = loadTexture("Background", m_mainRenderer);
    m_PIECES = loadTexture("Pieces", m_mainRenderer);
}

void Presenter::draw() {
    SDL_RenderPresent(m_mainRenderer);
    SDL_RenderClear(m_mainRenderer);
}

void Presenter::drawObject(SDL_Texture* texture) {
    SDL_RenderCopy(m_mainRenderer, texture, NULL, NULL);
}

void Presenter::drawObject(Drawable& drawable) {
    SDL_RenderCopy(m_mainRenderer, drawable.m_texture, NULL, &drawable.m_rect);
}
void Presenter::drawObject(SDL_Texture* texture, SDL_Rect rect) {
    SDL_RenderCopy(m_mainRenderer, texture, NULL, &rect);
}

void Presenter::drawPiece(PieceType type, SDL_Rect& rect, bool black) {
    SDL_Rect spriteRect = { int(type) * 334, black * 334, 334, 334 };
    SDL_RenderCopy(m_mainRenderer, m_PIECES, &spriteRect, &rect);
}
