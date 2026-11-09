#include "app/MainMenuScene.hpp"
#include "app/AudioController.hpp"
#include "app/LevelCatalog.hpp"
#include "app/CustomLevelCatalog.hpp"
#include "app/SceneManager.hpp"
#include "app/game/Level.hpp"
#include "ui/UiButton.hpp"
#include <raylib.h>
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <utility>

namespace {

constexpr int MaxLevelNameCharacters = 24;
std::size_t CountCharacters(const std::string& text) {
    return static_cast<std::size_t>(std::count_if(
        text.begin(),
        text.end(),
        [](unsigned char c) { return (c & 0xC0) != 0x80; }
    ));
}

void AppendCodepoint(std::string& text, int codepoint) {
    if (codepoint <= 0x7F) {
        text.push_back(static_cast<char>(codepoint));
    } else if (codepoint <= 0x7FF) {
        text.push_back(static_cast<char>(0xC0 | (codepoint >> 6)));
        text.push_back(static_cast<char>(0x80 | (codepoint & 0x3F)));
    } else if (codepoint <= 0xFFFF) {
        text.push_back(static_cast<char>(0xE0 | (codepoint >> 12)));
        text.push_back(static_cast<char>(0x80 | ((codepoint >> 6) & 0x3F)));
        text.push_back(static_cast<char>(0x80 | (codepoint & 0x3F)));
    }
}

void RemoveLastCharacter(std::string& text) {
    if (text.empty()) return;
    std::size_t start = text.size() - 1;
    while (start > 0 &&
           (static_cast<unsigned char>(text[start]) & 0xC0) == 0x80) {
        --start;
    }
    text.erase(start);
}

void StartLevel(AppContext* context, int level) {
    LevelDefinition definition = LevelDefinition::FromJsonPath(
        OfficialLevels[level].path
    );
    definition.levelNumber = level + 1;
    context->audioController->PlayUiClick();
    context->audioController->StartLevel(level + 1);
    context->sceneRequester->RequestSceneChange({SceneId::Game, &definition});
}

}

MainMenuScene::MainMenuScene(AppContext* context): m_context(context) {}

void MainMenuScene::ProcessInput(float) {
    if (m_context->returnToCustomDesign) {
        m_context->returnToCustomDesign = false;
        m_context->sceneRequester->RequestSceneChange({SceneId::CustomLevelEditor});
        return;
    }

    if (!m_selectingLevel) {
        if (IsKeyPressed(KEY_ENTER) || IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) m_selectingLevel = true;
        return;
    }

    if (m_editingLevels) {
        ProcessLevelEditorInput();
        return;
    }

    Rectangle resetButton{static_cast<float>(m_context->windowWidth - 130), 20.0f, 110.0f, 36.0f};
    if (UiButton::WasClicked(resetButton)) {
        m_context->audioController->PlayUiClick();
        m_context->unlockedLevel = 1;
        m_context->completedLevel = 0;
        m_context->audioController->ResetToMenu();
        m_selectingLevel = false;
        m_selectedLevel = 0;
        return;
    }

    const Rectangle editorButton = UiButton::CenteredAtBottom(
        m_context->windowWidth,
        m_context->windowHeight,
        230.0f,
        42.0f,
        22.0f
    );
    if (UiButton::WasClicked(editorButton)) {
        m_context->audioController->PlayUiClick();
        m_editingLevels = true;
        return;
    }

    if (m_context->completedLevel > 0) {
        m_context->unlockedLevel = std::max(
            m_context->unlockedLevel,
            std::min(OfficialLevelCount, m_context->completedLevel + 1)
        );
    }

    if (IsKeyPressed(KEY_LEFT)) m_selectedLevel = (m_selectedLevel + OfficialLevelCount - 1) % OfficialLevelCount;
    if (IsKeyPressed(KEY_RIGHT)) m_selectedLevel = (m_selectedLevel + 1) % OfficialLevelCount;
    if (IsKeyPressed(KEY_ENTER) && m_selectedLevel < m_context->unlockedLevel) StartLevel(m_context, m_selectedLevel);

    const float cardWidth = 200.0f;
    const float gap = 24.0f;
    const float startX = (m_context->windowWidth - (cardWidth * OfficialLevelCount + gap * (OfficialLevelCount - 1))) / 2.0f;
    for (int i = 0; i < OfficialLevelCount; ++i) {
        Rectangle card{startX + i * (cardWidth + gap), static_cast<float>(m_context->windowHeight) / 2 - 60, cardWidth, 120};
        if (i < m_context->unlockedLevel && UiButton::WasClicked(card)) {
            StartLevel(m_context, i);
        }
    }
}

void MainMenuScene::ProcessLevelEditorInput() {
    if (m_creatingLevel) {
        int codepoint = GetCharPressed();
        while (codepoint > 0) {
            if (codepoint >= 32 && codepoint != 127 &&
                CountCharacters(m_newLevelName) < MaxLevelNameCharacters) {
                AppendCodepoint(m_newLevelName, codepoint);
            }
            codepoint = GetCharPressed();
        }
        if (IsKeyPressed(KEY_BACKSPACE)) RemoveLastCharacter(m_newLevelName);
        if (IsKeyPressed(KEY_ESCAPE)) {
            m_creatingLevel = false;
            m_newLevelName.clear();
            return;
        }
        if (IsKeyPressed(KEY_ENTER) && !m_newLevelName.empty()) {
            m_context->customLevels->Add(m_newLevelName);
            m_newLevelName.clear();
            m_creatingLevel = false;
            return;
        }

        const Rectangle createButton{
            m_context->windowWidth / 2.0f + 10.0f,
            m_context->windowHeight / 2.0f + 48.0f,
            130.0f,
            38.0f
        };
        const Rectangle cancelButton{
            m_context->windowWidth / 2.0f - 140.0f,
            m_context->windowHeight / 2.0f + 48.0f,
            130.0f,
            38.0f
        };
        if (UiButton::WasClicked(cancelButton)) {
            m_creatingLevel = false;
            m_newLevelName.clear();
        } else if (UiButton::WasClicked(createButton) && !m_newLevelName.empty()) {
            m_context->customLevels->Add(m_newLevelName);
            m_newLevelName.clear();
            m_creatingLevel = false;
        }
        return;
    }

    const Rectangle createButton{
        (m_context->windowWidth - 210.0f) / 2.0f,
        92.0f,
        210.0f,
        42.0f
    };
    if (UiButton::WasClicked(createButton)) {
        m_context->audioController->PlayUiClick();
        m_newLevelName.clear();
        m_creatingLevel = true;
        return;
    }

    const Rectangle menuButton = UiButton::CenteredAtBottom(
        m_context->windowWidth,
        m_context->windowHeight,
        110.0f,
        38.0f,
        22.0f
    );
    if (UiButton::WasClicked(menuButton)) {
        m_context->audioController->PlayUiClick();
        m_editingLevels = false;
        return;
    }

    const float listTop = 154.0f;
    const float listBottom = static_cast<float>(m_context->windowHeight) - 76.0f;
    const float rowHeight = 50.0f;
    const float maxScroll = std::max(
        0.0f,
        m_context->customLevels->GetLevels().size() * rowHeight - (listBottom - listTop)
    );
    m_customListScroll = std::clamp(
        m_customListScroll - GetMouseWheelMove() * 40.0f,
        0.0f,
        maxScroll
    );

    const float rowWidth = std::min(620.0f, m_context->windowWidth - 40.0f);
    const float rowX = (m_context->windowWidth - rowWidth) / 2.0f;
    for (std::size_t i = 0; i < m_context->customLevels->GetLevels().size(); ++i) {
        const float rowY = listTop + i * rowHeight - m_customListScroll;
        if (rowY + rowHeight < listTop || rowY > listBottom) continue;
        const Rectangle deleteButton{rowX + rowWidth - 92.0f, rowY + 4.0f, 84.0f, 34.0f};
        const Rectangle editButton{rowX + rowWidth - 238.0f, rowY + 4.0f, 138.0f, 34.0f};
        if (UiButton::WasClicked(deleteButton)) {
            m_context->audioController->PlayUiClick();
            m_context->customLevels->Delete(i);
            return;
        }
        if (UiButton::WasClicked(editButton)) {
            m_context->audioController->PlayUiClick();
            m_context->activeCustomLevel = static_cast<int>(i);
            m_context->sceneRequester->RequestSceneChange({SceneId::CustomLevelEditor});
            return;
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
    } else if (!m_editingLevels) {
        const char* title = "Selecciona un nivel";
        DrawText(title, (m_context->windowWidth - MeasureText(title, 30)) / 2, m_context->windowHeight / 2 - 120, 30, BLACK);
        if (m_context->completedLevel > 0) {
            const char* message = TextFormat("¡Completaste el nivel %d! Ya puedes intentar el siguiente.", m_context->completedLevel);
            DrawText(message, (m_context->windowWidth - MeasureText(message, 18)) / 2, m_context->windowHeight / 2 - 82, 18, DARKGREEN);
        }
        Rectangle resetButton{static_cast<float>(m_context->windowWidth - 130), 20.0f, 110.0f, 36.0f};
        UiButton::Draw(resetButton, "Reiniciar", LIGHTGRAY, BLACK, 16, false, 18);

        const float cardWidth = 200.0f;
        const float gap = 24.0f;
        const float startX = (m_context->windowWidth - (cardWidth * OfficialLevelCount + gap * (OfficialLevelCount - 1))) / 2.0f;
        for (int i = 0; i < OfficialLevelCount; ++i) {
            Rectangle card{startX + i * (cardWidth + gap), static_cast<float>(m_context->windowHeight) / 2 - 60, cardWidth, 120};
            DrawRectangleRec(card, i < m_context->unlockedLevel ? SKYBLUE : GRAY);
            DrawRectangleLinesEx(card, 2, DARKGRAY);
            const char* level = OfficialLevels[i].label;
            DrawText(level, static_cast<int>(card.x + (card.width - MeasureText(level, 24)) / 2), static_cast<int>(card.y + 48), 24, BLACK);
        }
        UiButton::Draw(
            UiButton::CenteredAtBottom(
                m_context->windowWidth,
                m_context->windowHeight,
                230.0f,
                42.0f,
                22.0f
            ),
            "Editor de niveles"
        );
    } else if (m_editingLevels) {
        const char* title = "Editor de niveles";
        DrawText(title, (m_context->windowWidth - MeasureText(title, 30)) / 2, 38, 30, BLACK);
        UiButton::Draw(
            Rectangle{(m_context->windowWidth - 210.0f) / 2.0f, 92.0f, 210.0f, 42.0f},
            "Crear nivel",
            SKYBLUE
        );

        const float listTop = 154.0f;
        const float listBottom = static_cast<float>(m_context->windowHeight) - 76.0f;
        const float rowHeight = 50.0f;
        const float rowWidth = std::min(620.0f, m_context->windowWidth - 40.0f);
        const float rowX = (m_context->windowWidth - rowWidth) / 2.0f;
        bool drewLevel = false;
        for (std::size_t i = 0; i < m_context->customLevels->GetLevels().size(); ++i) {
            const float rowY = listTop + i * rowHeight - m_customListScroll;
            if (rowY + rowHeight < listTop || rowY > listBottom) continue;
            drewLevel = true;
            const Rectangle row{rowX, rowY, rowWidth, 42.0f};
            DrawRectangleRec(row, LIGHTGRAY);
            DrawRectangleLinesEx(row, 1.0f, DARKGRAY);
            const std::string& name = m_context->customLevels->GetLevels()[i].name;
            DrawText(name.c_str(), static_cast<int>(row.x + 14), static_cast<int>(row.y + 13), 16, BLACK);
            UiButton::Draw(
                Rectangle{rowX + rowWidth - 238.0f, rowY + 4.0f, 138.0f, 34.0f},
                "Editar y jugar",
                SKYBLUE,
                BLACK,
                14
            );
            UiButton::Draw(
                Rectangle{rowX + rowWidth - 92.0f, rowY + 4.0f, 84.0f, 34.0f},
                "Borrar",
                RED
            );
        }
        if (m_context->customLevels->GetLevels().empty()) {
            const char* emptyText = "Todavía no hay niveles personalizados";
            DrawText(
                emptyText,
                (m_context->windowWidth - MeasureText(emptyText, 18)) / 2,
                170,
                18,
                DARKGRAY
            );
        } else if (!drewLevel) {
            const char* scrollText = "Usa la rueda del mouse para ver más niveles";
            DrawText(scrollText, (m_context->windowWidth - MeasureText(scrollText, 16)) / 2, 170, 16, DARKGRAY);
        }

        UiButton::Draw(
            UiButton::CenteredAtBottom(
                m_context->windowWidth,
                m_context->windowHeight,
                110.0f,
                38.0f,
                22.0f
            ),
            "Menú"
        );

        if (m_creatingLevel) {
            DrawRectangle(0, 0, m_context->windowWidth, m_context->windowHeight, Fade(BLACK, 0.55f));
            const Rectangle dialog{
                (m_context->windowWidth - 500.0f) / 2.0f,
                (m_context->windowHeight - 210.0f) / 2.0f,
                500.0f,
                210.0f
            };
            DrawRectangleRec(dialog, RAYWHITE);
            DrawRectangleLinesEx(dialog, 2.0f, DARKGRAY);
            const char* prompt = "Nombre del nivel";
            DrawText(prompt, static_cast<int>(dialog.x + 24), static_cast<int>(dialog.y + 22), 20, BLACK);
            const Rectangle input{dialog.x + 24, dialog.y + 62, dialog.width - 48, 40};
            DrawRectangleRec(input, WHITE);
            DrawRectangleLinesEx(input, 1.0f, DARKGRAY);
            const char* placeholder = "Escribe un nombre...";
            const char* shownName = m_newLevelName.empty() ? placeholder : m_newLevelName.c_str();
            DrawText(shownName, static_cast<int>(input.x + 10), static_cast<int>(input.y + 11), 18, DARKGRAY);
            DrawText(
                TextFormat("%d/%d", static_cast<int>(CountCharacters(m_newLevelName)), MaxLevelNameCharacters),
                static_cast<int>(input.x + input.width - 58),
                static_cast<int>(input.y + 45),
                14,
                DARKGRAY
            );
            UiButton::Draw(
                Rectangle{dialog.x + 24, dialog.y + 148, 130, 38},
                "Cancelar"
            );
            UiButton::Draw(
                Rectangle{dialog.x + dialog.width - 154, dialog.y + 148, 130, 38},
                "Crear",
                m_newLevelName.empty() ? GRAY : SKYBLUE
            );
        }
    }

    EndDrawing();
}
