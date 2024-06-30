#include "Drawable.h"

Drawable::Drawable() {
	m_texture = nullptr;
	m_rect = {-1, -1, 0, 0};
	m_alpha = 255;
}

Drawable::~Drawable() {}