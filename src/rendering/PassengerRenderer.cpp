#include "rendering/PassengerRenderer.hpp"

#include <algorithm>
#include <cmath>

namespace {

// tamaño de la cara en relación a la escala del pasajero
constexpr float FaceSize = 18.0f;
// la cara se apoya sobre la cabeza dibujada en la textura del cuerpo
constexpr float HeadBottomLocalY = -13.0f;

// la textura del pasajero incluye cuerpo y cabeza; se dibuja completa
// alrededor de los pies (x, y)
constexpr float BodyWidthLocal = 16.0f;
constexpr float BodyHeightLocal = 37.0f;
constexpr float BodyCenterOffsetY = -12.5f;

bool IsStandardTint(Color tint) {
    return tint.r == BLACK.r && tint.g == BLACK.g && tint.b == BLACK.b;
}

Vector2 RotateAround(Vector2 point, Vector2 pivot, float sinR, float cosR) {
    const float dx = point.x - pivot.x;
    const float dy = point.y - pivot.y;
    return {
        pivot.x + dx * cosR - dy * sinR,
        pivot.y + dx * sinR + dy * cosR
    };
}

}

void DrawFaceTexture(
    const Texture2D& face,
    Rectangle dest,
    Color tint,
    float rotationDegrees
) {
    if (face.id == 0) return;
    const float side = static_cast<float>(std::min(face.width, face.height));
    const Rectangle source{
        (face.width - side) / 2.0f,
        (face.height - side) / 2.0f,
        side,
        side
    };
    DrawTexturePro(
        face,
        source,
        dest,
        Vector2{dest.width / 2.0f, dest.height / 2.0f},
        rotationDegrees,
        tint
    );
}

void DrawPassenger(
    const TextureLibrary& textureLibrary,
    float x,
    float y,
    bool hasSkirt,
    float scale,
    Color tint,
    const Texture2D* face,
    float rotationRadians
) {
    const bool standard = IsStandardTint(tint);
    const float sinR = std::sin(rotationRadians);
    const float cosR = std::cos(rotationRadians);
    const float rotationDegrees = rotationRadians * RAD2DEG;
    const Vector2 feet{x, y};

    const Texture2D& bodyTexture = hasSkirt
        ? textureLibrary.GetPassenngerSkirt()
        : textureLibrary.GetPassenngerSkirtless();
    if (bodyTexture.id != 0) {
        const float bodyWidth = BodyWidthLocal * scale;
        const float bodyHeight = BodyHeightLocal * scale;
        const Vector2 worldCenter = RotateAround(
            {feet.x, feet.y + BodyCenterOffsetY * scale},
            feet,
            sinR,
            cosR
        );
        DrawTexturePro(
            bodyTexture,
            Rectangle{
                0.0f,
                0.0f,
                static_cast<float>(bodyTexture.width),
                static_cast<float>(bodyTexture.height)
            },
            Rectangle{worldCenter.x, worldCenter.y, bodyWidth, bodyHeight},
            Vector2{bodyWidth / 2.0f, bodyHeight / 2.0f},
            rotationDegrees,
            tint
        );
    }

    if (face != nullptr && face->id != 0) {
        const float size = FaceSize * scale;
        const float localCenterY = (HeadBottomLocalY - FaceSize / 2.0f) * scale;
        const Vector2 worldCenter = RotateAround(
            {feet.x, feet.y + localCenterY},
            feet,
            sinR,
            cosR
        );
        // fantasma: la cara toma el color translúcido del fantasma
        DrawFaceTexture(
            *face,
            Rectangle{worldCenter.x, worldCenter.y, size, size},
            standard ? WHITE : tint,
            rotationDegrees
        );
    } else {
        const float localCenterY = (HeadBottomLocalY - 6.0f) * scale;
        const Vector2 worldCenter = RotateAround(
            {feet.x, feet.y + localCenterY},
            feet,
            sinR,
            cosR
        );
        DrawCircleV(worldCenter, 6.0f * scale, tint);
    }
}
