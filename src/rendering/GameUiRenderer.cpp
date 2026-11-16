#include "rendering/GameUiRenderer.hpp"

#include "app/game/PartDefinition.hpp"
#include "math/Vector2D.hpp"
#include "ui/UiButton.hpp"
#include "ui/UiLayout.hpp"

#include <algorithm>
#include <cmath>
#include <format>
#include <optional>
#include <string>

namespace {

void DrawHotkeyBadge(Rectangle bounds, int hotkey) {
    const Rectangle badge{
        bounds.x + bounds.width - 23.0f,
        bounds.y + 5.0f,
        18.0f,
        18.0f
    };
    DrawRectangleRec(badge, Fade(BLACK, 0.75f));
    DrawTextFont(
        TextFormat("%d", hotkey),
        static_cast<int>(badge.x + 5.0f),
        static_cast<int>(badge.y + 2.0f),
        12,
        WHITE
    );
}

void DrawCenteredTexture(const Texture2D& texture, Rectangle bounds, float padding, Color tint) {
    if (texture.id == 0) return;
    const float maxW = std::max(1.0f, bounds.width - padding * 2.0f);
    const float maxH = std::max(1.0f, bounds.height - padding * 2.0f);
    const float scale = std::min(maxW / texture.width, maxH / texture.height);
    const float w = texture.width * scale;
    const float h = texture.height * scale;
    UiButton::DrawTextureFit(
        texture,
        Rectangle{
            bounds.x + (bounds.width - w) / 2.0f,
            bounds.y + (bounds.height - h) / 2.0f,
            w,
            h
        },
        tint
    );
}

void DrawActionTexture(const Texture2D& texture, Rectangle bounds, float rotation) {
    if (texture.id == 0) return;
    const float size = std::min(bounds.width - 16.0f, bounds.height - 6.0f);
    DrawTexturePro(
        texture,
        Rectangle{0.0f, 0.0f,
            static_cast<float>(texture.width), static_cast<float>(texture.height)},
        Rectangle{
            bounds.x + bounds.width / 2.0f,
            bounds.y + bounds.height / 2.0f - 3.0f,
            size, size
        },
        Vector2{size / 2.0f, size / 2.0f},
        rotation * RAD2DEG,
        Color{255, 255, 255, 175}
    );
}

PartType ToolbarToPartType(ToolbarPart part) {
    return static_cast<PartType>(part);
}

const UiButtonSkin& SkinForEditorAction(const UiGameTextures& game, EditorAction action) {
    switch (action) {
        case EditorAction::MoveLeft: return game.moveLeft;
        case EditorAction::MoveRight: return game.moveRight;
        case EditorAction::MoveUp: return game.moveUp;
        case EditorAction::MoveDown: return game.moveDown;
        case EditorAction::ClearBuild: return game.clearBuild;
        case EditorAction::ClearSelection: return game.clearSelection;
        case EditorAction::EnterView: return game.enterView;
        case EditorAction::StartPlay: return game.modePlay;
        case EditorAction::ReturnToBuild: return game.modeEdit;
    }
    return game.modeEdit;
}

void DrawTexturedControl(
    const UiControl& control,
    const UiButtonSkin& skin,
    const UiGameTextures& gameUi,
    bool selected,
    std::optional<const Texture2D*> contentTexture,
    std::optional<float> contentRotation,
    std::optional<int> hotkey
) {
    const bool hovered = control.enabled && UiButton::IsHovered(control.bounds);
    const bool pressed = hovered && IsMouseButtonDown(MOUSE_BUTTON_LEFT);
    const bool driverBoxControl = control.type == UiControlType::ToolbarPart &&
        control.toolbarPart == ToolbarPart::DriverBox;
    const Texture2D& frame = UiButton::ResolveButtonTexture(skin, control.enabled, hovered, pressed);
    if (driverBoxControl) {
        DrawRectangleRec(control.bounds, Fade(RED, pressed ? 0.45f : hovered ? 0.35f : 0.25f));
    } else if (frame.id != 0) {
        UiButton::DrawTextureFit(frame, control.bounds, WHITE);
    } else {
        Color color = control.color;
        if (!control.enabled) color = Color{70, 70, 70, 255};
        DrawRectangleRec(control.bounds, color);
        DrawRectangleLinesEx(control.bounds, 1.0f, DARKGRAY);
    }

    if (driverBoxControl) {
        DrawRectangleLinesEx(control.bounds, selected ? 3.0f : 2.0f, RED);
    } else if (selected) {
        if (gameUi.toolbarSelected.id != 0) {
            UiButton::DrawTextureFit(gameUi.toolbarSelected, control.bounds, WHITE);
        } else {
            DrawRectangleLinesEx(control.bounds, 3.0f, YELLOW);
        }
    } else if (hovered && control.enabled) {
        DrawRectangleLinesEx(control.bounds, 2.0f, Fade(WHITE, 0.6f));
    }

    if (control.type == UiControlType::Activation && control.active) {
        DrawRectangleRec(
            control.bounds,
            Fade(IsFanAction(control.activationAction.value()) ? GREEN : ORANGE, 0.18f)
        );
    }

    if (contentTexture.has_value() && contentTexture.value() != nullptr &&
        contentTexture.value()->id != 0) {
        if (control.type == UiControlType::Activation) {
            DrawActionTexture(
                *contentTexture.value(), control.bounds,
                contentRotation.value_or(0.0f)
            );
        } else {
            DrawCenteredTexture(
                *contentTexture.value(), control.bounds, 5.0f,
                control.enabled ? WHITE : Color{140, 140, 140, 220}
            );
        }
    }
    if (control.type == UiControlType::Activation) {
        DrawRectangleRec(
            Rectangle{
                control.bounds.x + 2.0f,
                control.bounds.y + control.bounds.height - 20.0f,
                control.bounds.width - 4.0f,
                18.0f
            },
            Fade(BLACK, 0.65f)
        );
    }

    // label overlay (kept for clarity; can be hidden when art includes text)
    if (!control.label.empty()) {
        const char* label = control.label.data();
        int fontSize = control.labelFontSize;
        if (control.type == UiControlType::ToolbarPart) fontSize = 11;
        int textX = static_cast<int>(control.bounds.x) + control.labelOffsetX;
        int textY = static_cast<int>(control.bounds.y) + control.labelOffsetY;
        if (control.type == UiControlType::ToolbarPart) {
            textX = static_cast<int>(control.bounds.x + (control.bounds.width - MeasureTextFont(label, fontSize)) / 2.0f);
            textY = static_cast<int>(control.bounds.y + control.bounds.height - fontSize - 4);
        }
        DrawTextFont(label, textX + 1, textY + 1, fontSize, Fade(BLACK, 0.6f));
        DrawTextFont(label, textX, textY, fontSize, control.enabled ? WHITE : GRAY);
    }

    if (hotkey.has_value()) {
        DrawHotkeyBadge(control.bounds, hotkey.value());
    }
}

}

void GameUiRenderer::Render(
    int screenWidth,
    int screenHeight,
    GameState state,
    const BuildSession& buildSession,
    const EditorGrid& grid,
    const Camera2D& camera,
    const ActivationRegistry& activationRegistry,
    const TextureLibrary& textures
) const {
    (void)camera;
    DrawRectangle(
        0, static_cast<int>(screenHeight - UIBarHeight), screenWidth,
        static_cast<int>(UIBarHeight), Fade(BLACK, 0.95f)
    );
    DrawLineEx(
        {0, static_cast<float>(screenHeight - UIBarHeight)},
        {static_cast<float>(screenWidth), static_cast<float>(screenHeight - UIBarHeight)},
        5, {237, 18, 121,255}
    );

    const auto controls = UiLayout::BuildControls(
        state,
        activationRegistry,
        grid,
        screenWidth,
        screenHeight
    );
    const auto& selectedPartType = buildSession.GetSelectedPartType();
    const UiGameTextures& gameUi = textures.GetUi().game;

    int activationIndex = 0;
    int toolbarIndex = 0;
    for (const UiControl& control : controls) {
        bool selected = false;
        std::optional<int> hotkey;
        std::optional<const Texture2D*> contentTexture;
        std::optional<float> contentRotation;
        const UiButtonSkin* skin = &gameUi.toolbarSlot;

        if (control.type == UiControlType::ToolbarPart) {
            ++toolbarIndex;
            hotkey = toolbarIndex;
            if (control.toolbarPart.has_value()) {
                const PartType partType = ToolbarToPartType(control.toolbarPart.value());
                contentTexture = &textures.GetPartTexture(partType);
                if (selectedPartType.has_value() && selectedPartType.value() == partType) {
                    selected = true;
                }
            }
            skin = &gameUi.toolbarSlot;
        } else if (control.type == UiControlType::Activation) {
            ++activationIndex;
            hotkey = activationIndex == 10 ? 0 : activationIndex;
            selected = control.active;
            skin = &gameUi.toolbarSlot;
            const auto& subscribers = activationRegistry.GetSubscribers(
                control.activationAction.value()
            );
            if (!subscribers.empty() && subscribers.front() != nullptr) {
                const Part* part = subscribers.front();
                contentTexture = &textures.GetPartTexture(part->GetType());
                contentRotation = static_cast<float>(
                    part->GetRenderState(grid).rotation
                );
            }
        } else if (control.type == UiControlType::Editor && control.editorAction.has_value()) {
            skin = &SkinForEditorAction(gameUi, control.editorAction.value());
        }

        DrawTexturedControl(
            control, *skin, gameUi, selected, contentTexture,
            contentRotation, hotkey
        );
    }

    // ENTER badge for main mode button (play/edit side button) when present
    if (state == GameState::Build || state == GameState::Play) {
        const Rectangle modeButton = UiButton::PlaySideButton(screenWidth, screenHeight);
        const Texture2D& badgeTex = textures.GetUi().keyEnterBadge;
        const Rectangle enterBadge{
            modeButton.x + modeButton.width - 80.0f,
            modeButton.y + 4.0f,
            76.0f,
            32.0f
        };
        if (badgeTex.id != 0) {
            UiButton::DrawTextureFit(badgeTex, enterBadge);
        } else {
            DrawRectangleRec(enterBadge, Fade(BLACK, 0.65f));
            DrawTextFont("ENTER", static_cast<int>(enterBadge.x + 3.0f), static_cast<int>(enterBadge.y + 2.0f), 11, WHITE);
        }
    }

    if (state == GameState::Build) {
        const auto actions = ActivationRegistry::GetActionsForGrid(grid);
        if (!actions.empty()) {
            const int columns = 2;
            const int rows = (static_cast<int>(actions.size()) + columns - 1) / columns;
            const int panelX = screenWidth - 236;
            const int panelY = 80;
            const int panelHeight = 24 + rows * 22;
            DrawRectangle(panelX, panelY, 224, panelHeight, Fade(DARKGRAY, 0.82f));
            DrawTextFont("EN JUEGO (teclas 1-0)", panelX + 8, panelY + 5, 12, WHITE);
            for (std::size_t index = 0; index < actions.size(); ++index) {
                const int column = static_cast<int>(index) % columns;
                const int row = static_cast<int>(index) / columns;
                const int x = panelX + 8 + column * 106;
                const int y = panelY + 25 + row * 22;
                const int hotkey = index == 9 ? 0 : static_cast<int>(index) + 1;
                DrawTextFont(
                    TextFormat("%d %s", hotkey, GetActivationLabel(actions[index])),
                    x,
                    y,
                    12,
                    IsFanAction(actions[index]) ? ORANGE : GOLD
                );
            }
        }
    }

    std::string modeText, modeControlsText;
    Color modeTextColor;

    if (state == GameState::Build) {
        modeText = "MODO: DISEÑO";
        modeControlsText = "(Clic izq.: colocar, clic der.: borrar, ENTER: jugar)";
        modeTextColor = DARKGREEN;
    } else if (state == GameState::View) {
        modeText = "MODO: VISTA";
        modeControlsText = "(Arrastra para mover la cámara, ESC: diseñar)";
        modeTextColor = DARKPURPLE;
    } else {
        modeText = "MODO: JUEGO";
        modeControlsText = "(Pulsa ENTER para volver al diseño)";
        modeTextColor = MAROON;
    }

    const int modeTextFontSize = 30;
    const int modeControlsFontSize = 18;
    const int modeTextX = 20;
    const int modeTextY = 20;
    const int verticalPadding = 4;
    const int modeControlsTextY = modeTextY + verticalPadding + modeTextFontSize;

    DrawTextFont(modeText.c_str(), modeTextX, modeTextY, modeTextFontSize, modeTextColor);
    DrawTextFont(modeControlsText.c_str(), modeTextX, modeControlsTextY, modeControlsFontSize, modeTextColor);
}
