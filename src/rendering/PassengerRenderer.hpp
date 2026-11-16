#ifndef RENDERING_PASSENGER_RENDERER_HPP
#define RENDERING_PASSENGER_RENDERER_HPP

#include <raylib.h>

// Dibuja la textura recortada al cuadrado central (para fotos que no son cuadradas).
void DrawFaceTexture(const Texture2D& face, Rectangle dest, Color tint = WHITE);

// (x, y) son los pies del pasajero. Si `face` no es nulo, la cabeza es esa imagen.
void DrawPassenger(
    float x,
    float y,
    bool hasSkirt,
    float scale = 1.0f,
    Color tint = BLACK,
    const Texture2D* face = nullptr
);

#endif
