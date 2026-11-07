#include "app/MainMenuScene.hpp"
#include "app/SceneManager.hpp"
#include "app/game/Level.hpp"
#include <raylib.h>
#include <algorithm>

namespace {

void StartLevel(AppContext* context, int level) {
    const char* paths[] = {
        "assets/levels/level_1.json",
        "assets/levels/level_2.json",
        "assets/levels/level_3.json"
    };
    LevelDefinition definition = LevelDefinition::FromJsonPath(paths[level]);
    definition.levelNumber = level + 1;
    context->sceneRequester->RequestSceneChange({SceneId::Game, &definition});
}

}

void MainMenuScene::ProcessInput(float) {
    if (!m_selectingLevel) {
        if (IsKeyPressed(KEY_ENTER) || IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) m_selectingLevel = true;
        return;
    }

    Rectangle resetButton{static_cast<float>(m_context->windowWidth - 130), 20.0f, 110.0f, 36.0f};
    if (CheckCollisionPointRec(GetMousePosition(), resetButton) &&
        IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        m_context->unlockedLevel = 1;
        m_context->completedLevel = 0;
        m_selectingLevel = false;
        m_selectedLevel = 0;
        return;
    }

    if (m_context->completedLevel > 0) {
        m_context->unlockedLevel = std::max(
            m_context->unlockedLevel,
            std::min(3, m_context->completedLevel + 1)
        );
    }

    if (IsKeyPressed(KEY_LEFT)) m_selectedLevel = (m_selectedLevel + 2) % 3;
    if (IsKeyPressed(KEY_RIGHT)) m_selectedLevel = (m_selectedLevel + 1) % 3;
    if (IsKeyPressed(KEY_ENTER) && m_selectedLevel < m_context->unlockedLevel) StartLevel(m_context, m_selectedLevel);

    const float cardWidth = 200.0f;
    const float gap = 24.0f;
    const float startX = (m_context->windowWidth - (cardWidth * 3 + gap * 2)) / 2.0f;
    for (int i = 0; i < 3; ++i) {
        Rectangle card{startX + i * (cardWidth + gap), static_cast<float>(m_context->windowHeight) / 2 - 60, cardWidth, 120};
        if (i < m_context->unlockedLevel && CheckCollisionPointRec(GetMousePosition(), card) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            StartLevel(m_context, i);
        }
    }
}

void MainMenuScene::Render() const {
    BeginDrawing();
    ClearBackground(RAYWHITE);
    if (!m_selectingLevel) {
        const char* title = "Tunea tu combi";
        DrawText(title, (m_context->windowWidth - MeasureText(title, 40)) / 2, m_context->windowHeight / 2 - 40, 40, BLACK);
        const char* prompt = "Presiona ENTER o haz clic para continuar";
        DrawText(prompt, (m_context->windowWidth - MeasureText(prompt, 20)) / 2, m_context->windowHeight / 2 + 30, 20, DARKGRAY);
    } else {
        const char* title = "Selecciona un nivel";
        DrawText(title, (m_context->windowWidth - MeasureText(title, 30)) / 2, m_context->windowHeight / 2 - 120, 30, BLACK);
        if (m_context->completedLevel > 0) {
            const char* message = TextFormat("¡Completaste el nivel %d! Ya puedes intentar el siguiente.", m_context->completedLevel);
            DrawText(message, (m_context->windowWidth - MeasureText(message, 18)) / 2, m_context->windowHeight / 2 - 82, 18, DARKGREEN);
        }
        Rectangle resetButton{static_cast<float>(m_context->windowWidth - 130), 20.0f, 110.0f, 36.0f};
        DrawRectangleRec(resetButton, LIGHTGRAY);
        DrawRectangleLinesEx(resetButton, 1, DARKGRAY);
        DrawText("Reiniciar", static_cast<int>(resetButton.x + 18), 30, 16, BLACK);
        const float cardWidth = 200.0f;
        const float gap = 24.0f;
        const float startX = (m_context->windowWidth - (cardWidth * 3 + gap * 2)) / 2.0f;
        for (int i = 0; i < 3; ++i) {
            Rectangle card{startX + i * (cardWidth + gap), static_cast<float>(m_context->windowHeight) / 2 - 60, cardWidth, 120};
            DrawRectangleRec(card, i < m_context->unlockedLevel ? SKYBLUE : GRAY);
            DrawRectangleLinesEx(card, 2, DARKGRAY);
            const char* level = TextFormat("Nivel %d", i + 1);
            DrawText(level, static_cast<int>(card.x + (card.width - MeasureText(level, 24)) / 2), static_cast<int>(card.y + 48), 24, BLACK);
        }
    }
    EndDrawing();
}
