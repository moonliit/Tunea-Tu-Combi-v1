#include "rendering/PassengerRenderer.hpp"

#include <algorithm>

namespace {

// tamaño de la cara en relación a la escala del pasajero
constexpr float FaceSize = 18.0f;

bool IsStandardTint(Color tint) {
    return tint.r == BLACK.r && tint.g == BLACK.g && tint.b == BLACK.b;
}

}

void DrawFaceTexture(const Texture2D& face, Rectangle dest, Color tint) {
    if (face.id == 0) return;
    const float side = static_cast<float>(std::min(face.width, face.height));
    const Rectangle source{
        (face.width - side) / 2.0f,
        (face.height - side) / 2.0f,
        side,
        side
    };
    DrawTexturePro(face, source, dest, Vector2{0.0f, 0.0f}, 0.0f, tint);
}

void DrawPassenger(
    float x,
    float y,
    bool hasSkirt,
    float scale,
    Color tint,
    const Texture2D* face
) {
    const bool standard = IsStandardTint(tint);

    if (face != nullptr && face->id != 0) {
        const float size = FaceSize * scale;
        // la parte de abajo de la cara toca el cuello (y - 13)
        const Rectangle dest{
            x - size / 2.0f,
            y - 13.0f * scale - size,
            size,
            size
        };
        // fantasma: la cara toma el color translúcido del fantasma
        DrawFaceTexture(*face, dest, standard ? WHITE : tint);
    } else {
        DrawCircleV({x, y - 19.0f * scale}, 6.0f * scale, tint);
    }

    DrawLineEx({x, y - 13.0f * scale}, {x, y - 2.0f * scale}, 3.0f * scale, tint);
    DrawLineEx({x, y - 9.0f * scale}, {x - 7.0f * scale, y - 4.0f * scale}, 2.0f * scale, tint);
    DrawLineEx({x, y - 9.0f * scale}, {x + 7.0f * scale, y - 4.0f * scale}, 2.0f * scale, tint);
    if (hasSkirt) {
        DrawTriangle({x - 5.0f * scale, y - 2.0f * scale},
            {x + 5.0f * scale, y - 2.0f * scale},
            {x, y + 4.0f * scale}, standard ? PINK : tint);
    }
    DrawLineEx({x, y - 2.0f * scale}, {x - 4.0f * scale, y + 5.0f * scale}, 2.0f * scale, tint);
    DrawLineEx({x, y - 2.0f * scale}, {x + 4.0f * scale, y + 5.0f * scale}, 2.0f * scale, tint);
}
