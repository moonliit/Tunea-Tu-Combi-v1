#ifndef UI_BUTTON_HPP
#define UI_BUTTON_HPP

#include "rendering/TextureLibrary.hpp"
#include <raylib.h>

namespace UiButton {

inline Rectangle PlaySideButton(int windowWidth, int windowHeight) {
    const float sidePadding = 20.0f;
    const float width = 170.0f;
    const float height = 150.0f;

    return Rectangle{
        static_cast<float>(windowWidth - width - sidePadding),
        static_cast<float>((windowHeight - height)/ 2),
        width,
        height
    };
}

inline Rectangle CenteredAtBottom(
    int windowWidth,
    int windowHeight,
    float width,
    float height,
    float bottomOffset
) {
    return Rectangle{
        (windowWidth - width) / 2.0f,
        static_cast<float>(windowHeight) - bottomOffset - height,
        width,
        height
    };
}

inline Rectangle AtTopRight(
    int windowWidth,
    float rightMargin,
    float topOffset,
    float width,
    float height
) {
    return Rectangle{
        static_cast<float>(windowWidth) - rightMargin - width,
        topOffset,
        width,
        height
    };
}

inline Rectangle GameMenuButton(int windowWidth) {
    return AtTopRight(windowWidth, 20.0f, 20.0f, 151.0f, 52.0f);
}

inline Rectangle CenteredAbove(
    Rectangle reference,
    float width,
    float height,
    float gap
) {
    return Rectangle{
        reference.x + reference.width  - width - gap,
        reference.y - gap - height,
        width,
        height
    };
}

inline bool WasClicked(Rectangle bounds) {
    return CheckCollisionPointRec(GetMousePosition(), bounds) &&
        IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
}

inline bool IsHovered(Rectangle bounds) {
    return CheckCollisionPointRec(GetMousePosition(), bounds);
}

inline void DrawHotkeyNumber(Rectangle bounds, int hotkey) {
    const char* label = TextFormat("%d", hotkey);
    constexpr int fontSize = 12;
    const int x = static_cast<int>(bounds.x + bounds.width -
        MeasureTextFont(label, fontSize) - 8.0f);
    const int y = static_cast<int>(bounds.y + 5.0f);
    DrawTextFont(label, x + 1, y + 1, fontSize, Fade(BLACK, 0.8f));
    DrawTextFont(label, x, y, fontSize, WHITE);
}

inline void DrawTextureFit(const Texture2D& texture, Rectangle bounds, Color tint = WHITE) {
    if (texture.id == 0) return;
    DrawTexturePro(
        texture,
        Rectangle{0.0f, 0.0f, static_cast<float>(texture.width), static_cast<float>(texture.height)},
        bounds,
        Vector2{0.0f, 0.0f},
        0.0f,
        tint
    );
}

inline const Texture2D& ResolveButtonTexture(
    const UiButtonSkin& skin,
    bool enabled = true,
    bool hovered = false,
    bool pressed = false
) {
    if (!enabled) return skin.disabled;
    if (pressed) return skin.pressed;
    if (hovered) return skin.hover;
    return skin.normal;
}

inline void Draw(
    Rectangle bounds,
    const char* label,
    Color background = LIGHTGRAY,
    Color foreground = BLACK,
    int fontSize = 18,
    bool centerText = true,
    int leftPadding = 8,
    bool drawBorder = true
) {
    DrawRectangleRec(bounds, background);
    if (drawBorder) DrawRectangleLinesEx(bounds, 1.0f, DARKGRAY);
    const int textX = centerText
        ? static_cast<int>(bounds.x +
            (bounds.width - MeasureTextFont(label, fontSize)) / 2)
        : static_cast<int>(bounds.x) + leftPadding;
    DrawTextFont(
        label,
        textX,
        static_cast<int>(bounds.y + (bounds.height - fontSize) / 2),
        fontSize,
        foreground
    );
}

inline void DrawTextured(
    Rectangle bounds,
    const UiButtonSkin& skin,
    bool enabled = true,
    const char* fallbackLabel = nullptr,
    Color fallbackBackground = LIGHTGRAY,
    Color fallbackForeground = BLACK,
    int fontSize = 18
) {
    const bool hovered = enabled && IsHovered(bounds);
    const bool pressed = hovered && IsMouseButtonDown(MOUSE_BUTTON_LEFT);
    const Texture2D& texture = ResolveButtonTexture(skin, enabled, hovered, pressed);
    if (texture.id != 0) {
        DrawTextureFit(texture, bounds, WHITE);
    } else {
        Draw(bounds, fallbackLabel ? fallbackLabel : "", fallbackBackground, fallbackForeground, fontSize);
    }
}

}

#endif
