#ifndef INPUT_EDITOR_INPUT_HPP
#define INPUT_EDITOR_INPUT_HPP

#include "math/Vector2D.hpp"
#include "app/editor/EditorState.hpp"

struct EditorInputFrame {
    bool toggleMode = false;
    bool placePart = false;
    bool removePart = false;
    bool rotatePart = false;
    bool rotatePlacedPart = false;
    bool pickupPart = false;
    bool clearSelection = false;
    bool cameraDragging = false;
    Math::Vector2D cameraMovement = Math::Vector2D::Zero();
    Math::Vector2D mouseScreenPosition = Math::Vector2D::Zero();
};

class EditorInput {
public:
    EditorInputFrame Capture(
        float deltaTime,
        EditorState state,
        const Camera2D& camera,
        int screenWidth,
        int screenHeight
    ) const;

private:
    mutable bool m_dragging = false;
    mutable bool m_pendingWorldClick = false;
    mutable bool m_canDrag = false;
    mutable Math::Vector2D m_pressScreenPosition = Math::Vector2D::Zero();
    mutable bool m_repositioningPart = false;
};

#endif
