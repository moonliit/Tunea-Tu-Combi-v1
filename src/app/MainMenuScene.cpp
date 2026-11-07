#include "app/MainMenuScene.hpp"
#include "app/SceneManager.hpp"
#include <raylib.h>

void MainMenuScene::ProcessInput(float) {
    if (!m_selectingLevel) {
        if (IsKeyPressed(KEY_ENTER) || IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) m_selectingLevel = true;
        return;
    }

    Rectangle levelCard{static_cast<float>(m_context->windowWidth) / 2 - 100, static_cast<float>(m_context->windowHeight) / 2 - 60, 200, 120};
    if (CheckCollisionPointRec(GetMousePosition(), levelCard) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        m_context->sceneRequester->RequestSceneChange({SceneId::Game});
    }
    if (IsKeyPressed(KEY_ENTER)) m_context->sceneRequester->RequestSceneChange({SceneId::Game});
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
        Rectangle levelCard{static_cast<float>(m_context->windowWidth) / 2 - 100, static_cast<float>(m_context->windowHeight) / 2 - 60, 200, 120};
        DrawRectangleRec(levelCard, LIGHTGRAY);
        DrawRectangleLinesEx(levelCard, 2, DARKGRAY);
        const char* level = "Nivel 1";
        DrawText(level, (m_context->windowWidth - MeasureText(level, 24)) / 2, m_context->windowHeight / 2 - 12, 24, BLACK);
    }
    EndDrawing();
}
