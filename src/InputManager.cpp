#include "InputManager.h"

InputManager::InputManager() {
    m_mouseDown = false;
    m_mouseMoved = false;
    m_mouseUp = false;
    m_keyReleased = false;
    m_quit = false;
    m_mouseHeld = false;
    keyPressed = '\0';
}

InputManager::~InputManager() {}

void InputManager::getInput() {
    m_mouseDown = false;
    m_mouseMoved = false;
    m_mouseUp = false;
    m_keyReleased = false;
    m_quit = false;
    keyPressed = '\0';
    SDL_Event m_event;
    const Uint8* curKeyStates = SDL_GetKeyboardState(NULL);
    while (SDL_PollEvent(&m_event)) {
        switch (m_event.type) {
        case SDL_QUIT:
            m_quit = true;
            break;

        case SDL_MOUSEMOTION:
            SDL_GetMouseState(&(m_mouseCoord.x), &(m_mouseCoord.y));
            m_mouseMoved = true;
            break;

        case SDL_MOUSEBUTTONDOWN:
            if (m_event.button.button == SDL_BUTTON_LEFT){
                m_mouseDown = true;
                m_mouseHeld = true;
            }
            break;
            
        case SDL_MOUSEBUTTONUP:
            if (m_event.button.button == SDL_BUTTON_LEFT) {
                m_mouseUp = true;
                m_mouseHeld = false;
            }
            break;
        case SDL_KEYUP:
            m_keyReleased = true;
            break;
        }
    
    }
    for (int i = SDL_SCANCODE_A; i <= SDL_SCANCODE_Z; i++) {
        if (curKeyStates[i]) {
            keyPressed = 'a' + i - SDL_SCANCODE_A;
        }
    }
}
