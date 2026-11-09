#ifndef UI_BUTTON_HPP
#define UI_BUTTON_HPP

#include <raylib.h>

namespace UiButton {

inline Rectangle PlaySideButton(int windowWidth, int windowHeight) {
    return Rectangle{
        static_cast<float>(windowWidth - 150),
        static_cast<float>(windowHeight / 2 - 45),
        130.0f,
        90.0f
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

inline Rectangle CenteredAbove(
    Rectangle reference,
    float width,
    float height,
    float gap
) {
    return Rectangle{
        reference.x + (reference.width - width) / 2.0f,
        reference.y - gap - height,
        width,
        height
    };
}

inline bool WasClicked(Rectangle bounds) {
    return CheckCollisionPointRec(GetMousePosition(), bounds) &&
        IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
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
            (bounds.width - MeasureText(label, fontSize)) / 2)
        : static_cast<int>(bounds.x) + leftPadding;
    DrawText(
        label,
        textX,
        static_cast<int>(bounds.y + (bounds.height - fontSize) / 2),
        fontSize,
        foreground
    );
}

}

#endif
