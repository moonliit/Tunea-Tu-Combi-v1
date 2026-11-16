#ifndef RENDERING_PASSENGER_RENDERER_HPP
#define RENDERING_PASSENGER_RENDERER_HPP

#include <raylib.h>
#include "rendering/TextureLibrary.hpp"

// Dibuja la textura recortada al cuadrado central (para fotos que no son cuadradas).
void DrawFaceTexture(
    const Texture2D& face,
    Rectangle dest,
    Color tint = WHITE,
    float rotationDegrees = 0.0f
);

// (x, y) son los pies del pasajero. Si `face` no es nulo, la cabeza es esa imagen.
// El cuerpo se dibuja con textura (con o sin falda) y puede rotar alrededor de los
// pies para seguir la inclinación del vehículo o girar al ser eyectado.
void DrawPassenger(
    const TextureLibrary& textureLibrary,
    float x,
    float y,
    bool hasSkirt,
    float scale = 1.0f,
    Color tint = BLACK,
    const Texture2D* face = nullptr,
    float rotationRadians = 0.0f
);

#endif
