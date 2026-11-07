#ifndef INPUT_GAME_INPUT_HPP
#define INPUT_GAME_INPUT_HPP

#include "math/Vector2D.hpp"
#include "app/game/ActivationRegistry.hpp"
#include "app/game/GameState.hpp"
#include "math/GridCoord.hpp"
#include "ui/UiControl.hpp"

#include <optional>

struct Camera2D;

struct GameInputFrame {
    bool toggleMode = false;
    bool placePart = false;
    bool removePart = false;
    bool rotatePart = false;
    bool rotatePlacedPart = false;
    bool pickupPart = false;
    bool clearSelection = false;
    bool discardRepositionedPart = false;
    Math::Vector2D cameraMovement = Math::Vector2D::Zero();
    Math::Vector2D mouseScreenPosition = Math::Vector2D::Zero();
    std::optional<Math::GridCoord> interactionCoord;
    std::optional<ToolbarPart> toolbarPart;
    std::optional<ActivationAction> activationAction;
    std::optional<EditorAction> editorAction;
    bool cameraDragging = false;
};

struct GameInputContext {
    float deltaTime;
    GameState state;
    const ActivationRegistry& activationRegistry;
    const EditorGrid& grid;
    const Camera2D& camera;
    const std::optional<PartType>& selectedPartType;
    bool hasRepositionedPart;
    int screenWidth;
    int screenHeight;
};

class GameInput {
public:
    GameInputFrame Capture(const GameInputContext& context) const;

private:
    mutable bool m_dragging = false;
    mutable bool m_pendingWorldClick = false;
    mutable bool m_canDrag = false;
    mutable Math::Vector2D m_pressScreenPosition = Math::Vector2D::Zero();
    mutable std::optional<Math::GridCoord> m_pressGridCoord;
    mutable bool m_repositioningPart = false;
};

#endif
