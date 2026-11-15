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
#include <print>

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
        OfficialLevels[level].jsonPath
    );
    definition.levelNumber = level + 1;
    context->audioController->PlayUiClick();
    context->audioController->StartLevel(level + 1);
    context->sceneRequester->RequestSceneChange({SceneId::Game, &definition});
}

void CreateCustomLevel(AppContext* context, const std::string& name) {
    context->customLevels->Add(name);
    context->activeCustomLevel = static_cast<int>(context->customLevels->GetLevels().size()) - 1;
    context->audioController->StartLevelEditorMusic(
        context->customLevels->GetLevels()[context->activeCustomLevel].musicPath, true
    );
    context->openCustomMusicPicker = true;
    context->sceneRequester->RequestSceneChange({SceneId::CustomLevelEditor});
}

}

MainMenuScene::MainMenuScene(AppContext* context):
    m_context(context),
    m_selectedLevel(std::clamp(context->unlockedLevel - 1, 0, OfficialLevelCount - 1)) {}

void MainMenuScene::Update(float) {
    if (auto copy = m_context->publications.TakeCopyRequest()) {
        if (m_context->customLevels->AddCopy(copy.value())) {
            m_editingLevels = true;
            m_customListScroll = 0.0f;
        }
    }
    if (auto play = m_context->publications.TakePlayRequest()) {
        try {
            LevelDefinition definition = LevelDefinition::FromJson(
                play->at("definition")
            );
            definition.levelNumber = -2;
            const std::string musicPath = play->at("music").at("path");
            m_context->audioController->StartLevelEditorMusic(musicPath, false);
            m_context->sceneRequester->RequestSceneChange({SceneId::Game, &definition});
        } catch (const nlohmann::json::exception&) {}
    }
}

void MainMenuScene::ProcessInput(float) {
    if (m_context->returnedToSelector) {
        m_context->returnedToSelector = false;
        m_selectedLevel = std::clamp(m_context->unlockedLevel - 1, 0, OfficialLevelCount - 1);
    }

    if (m_context->returnToCustomDesign) {
        m_context->returnToCustomDesign = false;
        m_context->sceneRequester->RequestSceneChange({SceneId::CustomLevelEditor});
        return;
    }

    if (m_editingLevels) {
        ProcessLevelEditorInput();
        return;
    }

    Rectangle menuButton{
        static_cast<float>(m_context->windowWidth - 230),
        20.0f,
        210.0f,
        69.0f
    };
    if (UiButton::WasClicked(menuButton)) {
        m_context->audioController->PlayUiClick();
        m_context->audioController->ResetToMenu();
        m_selectingLevel = false;
        m_selectedLevel = 0;
        return;
    }

    const float buttonWidth = 145.0f;
    const float buttonHeight = 145.0f;
    const float buttonY = m_context->windowHeight / 2.0f + 22.0f;
    const float separationX = 100.0f;
    const float sumWidth = buttonWidth * 2.0f + separationX;
    const float button1X = (m_context->windowWidth - sumWidth) / 2.0f;
    const float button2X = button1X + buttonWidth + separationX;

    const Rectangle playButton = Rectangle{
        button1X,
        buttonY,
        buttonWidth,
        buttonHeight
    };
    const Rectangle editorButton =  Rectangle{
        button2X,
        buttonY,
        buttonWidth,
        buttonHeight
    };

    if (UiButton::WasClicked(playButton) && !m_selectingLevel && !m_editingLevels) {
        m_context->audioController->PlayUiClick();
        m_context->audioController->SetCurrentScenario({Scenario::LevelSelect});
        m_selectingLevel = true;
        m_editingLevels = false;
        return;
    }
    if (UiButton::WasClicked(editorButton) && !m_selectingLevel && !m_editingLevels) {
        m_context->audioController->PlayUiClick();
        m_context->audioController->SetCurrentScenario({Scenario::LevelSelect});
        m_selectingLevel = true;
        m_editingLevels = true;
        return;
    }

    const int previousUnlockedLevel = m_context->unlockedLevel;
    if (m_context->completedLevel > 0) {
        m_context->unlockedLevel = std::max(
            m_context->unlockedLevel,
            std::min(OfficialLevelCount, m_context->completedLevel + 1)
        );
    }
    if (m_context->unlockedLevel > previousUnlockedLevel) {
        m_selectedLevel = m_context->unlockedLevel - 1;
    }

    if (!m_selectingLevel) return;

    const float cardWidth = std::min(200.0f, (m_context->windowWidth - 180.0f) / 3.0f);
    const float sideCardWidth = cardWidth * 0.68f;
    const float gap = 20.0f;
    const float cardGap = 20.0f;
    const float centerX = m_context->windowWidth / 2.0f;
    const float cardY = static_cast<float>(m_context->windowHeight) / 2.0f - 60.0f;

    const Rectangle leftArrow{
        centerX - cardWidth / 2.0f - gap - sideCardWidth - 72.0f,
        cardY + 38.0f,
        60.0f,
        66.0f
    };
    const Rectangle rightArrow{
        centerX + cardWidth / 2.0f + gap + sideCardWidth + 16.0f,
        cardY + 38.0f,
        60.0f,
        66.0f
    };
    if (IsKeyPressed(KEY_LEFT) || UiButton::WasClicked(leftArrow))
        m_selectedLevel = (m_selectedLevel + OfficialLevelCount - 1) % OfficialLevelCount;
    if (IsKeyPressed(KEY_RIGHT) || UiButton::WasClicked(rightArrow))
        m_selectedLevel = (m_selectedLevel + 1) % OfficialLevelCount;
    if (IsKeyPressed(KEY_ENTER) && m_selectedLevel < m_context->unlockedLevel) StartLevel(m_context, m_selectedLevel);

    for (int offset = -1; offset <= 1; ++offset) {
        const int levelIndex = (m_selectedLevel + offset + OfficialLevelCount) % OfficialLevelCount;
        const float width = offset == 0 ? cardWidth : sideCardWidth;
        const float height = offset == 0 ? 120.0f : 82.0f;
        const float cardCenterX = centerX + offset * (cardWidth / 2.0f + sideCardWidth / 2.0f + cardGap);
        Rectangle card{cardCenterX - width / 2.0f, cardY + (120.0f - height) / 2.0f, width, height};
        if (levelIndex < m_context->unlockedLevel && UiButton::WasClicked(card)) {
            StartLevel(m_context, levelIndex);
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
            CreateCustomLevel(m_context, m_newLevelName);
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
            CreateCustomLevel(m_context, m_newLevelName);
            m_newLevelName.clear();
            m_creatingLevel = false;
        }
        return;
    }

    const Rectangle createButton{
        (m_context->windowWidth - 210.0f) / 2.0f,
        92.0f,
        210.0f,
        70.0f
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
        210.0f,
        70.0f,
        22.0f
    );
    if (UiButton::WasClicked(menuButton)) {
        m_context->audioController->PlayUiClick();
        m_context->audioController->ResetToMenu();
        m_selectingLevel = false;
        m_editingLevels = false;
        m_selectedLevel = 0;
        return;
    }

    const float listTop = 182.0f;
    const float listBottom = static_cast<float>(m_context->windowHeight) - 76.0f;
    const float rowHeight = 75.0f;
    const float maxScroll = std::max(
        0.0f,
        m_context->customLevels->GetLevels().size() * rowHeight - (listBottom - listTop)
    );
    m_customListScroll = std::clamp(
        m_customListScroll - GetMouseWheelMove() * 40.0f,
        0.0f,
        maxScroll
    );

    const float yPadding = 15.0f;
    const float rowWidth = std::min(820.0f, m_context->windowWidth - 40.0f);
    const float rowX = (m_context->windowWidth - rowWidth) / 2.0f;

    for (std::size_t i = 0; i < m_context->customLevels->GetLevels().size(); ++i) {
        const float rowY = listTop + i * (rowHeight + yPadding) - m_customListScroll;
        if (rowY + rowHeight < listTop || rowY > listBottom) continue;

        const Rectangle editButton{rowX + rowWidth - 260.0f, rowY + 9.0f, 120.0f, 57.0f};
        const Rectangle publishButton{rowX + rowWidth - 398.0f, rowY + 9.0f, 120.0f, 57.0f};
        const Rectangle deleteButton{rowX + rowWidth - 132.0f, rowY + 9.0f, 120.0f, 57.0f};

        if (UiButton::WasClicked(deleteButton)) {
            m_context->audioController->PlayUiClick();
            m_context->customLevels->Delete(i);
            return;
        }
        if (UiButton::WasClicked(editButton)) {
            m_context->audioController->PlayUiClick();
            m_context->activeCustomLevel = static_cast<int>(i);
            m_context->audioController->StartLevelEditorMusic(
                m_context->customLevels->GetLevels()[i].musicPath, false
            );
            m_context->sceneRequester->RequestSceneChange({SceneId::CustomLevelEditor});
            return;
        }
        if (UiButton::WasClicked(publishButton) &&
            m_context->customLevels->HasValidApproval(i)) {
            m_context->audioController->PlayUiClick();
            m_context->publications.Publish(
                m_context->customLevels->GetPublicationPayload(i)
            );
            return;
        }
    }
}





void MainMenuScene::Render() const {
    BeginDrawing();
    ClearBackground(RAYWHITE);

    const UiTextures& ui = m_context->textureLibrary->GetUi();

    if (!m_selectingLevel) {
        const Texture2D& logo = ui.logo;
        const float maxLogoWidth = m_context->windowWidth * 0.7f;
        const float maxLogoHeight = m_context->windowHeight * 0.35f;
        float logoWidth = static_cast<float>(logo.width);
        float logoHeight = static_cast<float>(logo.height);
        if (logoWidth > 0.0f && logoHeight > 0.0f) {
            const float scale = std::min(maxLogoWidth / logoWidth, maxLogoHeight / logoHeight);
            logoWidth *= scale;
            logoHeight *= scale;
        } else {
            logoWidth = maxLogoWidth;
            logoHeight = maxLogoHeight * 0.5f;
        }
        UiButton::DrawTextureFit(
            logo,
            Rectangle{
                (m_context->windowWidth - logoWidth) / 2.0f,
                m_context->windowHeight/10.0f,
                logoWidth,
                logoHeight
            }
        );

        const float buttonWidth = 145.0f;
        const float buttonHeight = 145.0f;
        const float buttonY = m_context->windowHeight / 2.0f + 22.0f;
        const float separationX = 100.0f;
        const float sumWidth = buttonWidth * 2.0f + separationX;
        const float button1X = (m_context->windowWidth - sumWidth) / 2.0f;
        const float button2X = button1X + buttonWidth + separationX;

        UiButton::DrawTextured(
            Rectangle{
                button1X,
                buttonY,
                buttonWidth,
                buttonHeight
            },
            ui.game.modePlay,
            true,
            "Jugar"
        );
        UiButton::DrawTextured(
            Rectangle{
                button2X,
                buttonY,
                buttonWidth,
                buttonHeight
            },
            ui.menuToEditor,
            true,
            "Editor de niveles"
        );
    } else if (!m_editingLevels) {
        const char* title = "Selecciona un nivel";
        DrawTextFont(title, (m_context->windowWidth - MeasureText(title, 30)) / 2, m_context->windowHeight / 2 - 120, 30, BLACK);
        Rectangle menuButton{
            static_cast<float>(m_context->windowWidth - 230),
            20.0f,
            210.0f,
            69.0f
        };
        UiButton::DrawTextured(
            menuButton,
            ui.menuBack,
            true,
            "Menu"
        );

        const float cardWidth = std::min(200.0f, (m_context->windowWidth - 180.0f) / 3.0f);
        const float sideCardWidth = cardWidth * 0.68f;
        const float gap = 20.0f;
        const float centerX = m_context->windowWidth / 2.0f;
        const float cardY = static_cast<float>(m_context->windowHeight) / 2.0f - 60.0f;

        const Rectangle leftArrow{
            centerX - cardWidth / 2.0f - gap - sideCardWidth - 72.0f,
            cardY + 38.0f,
            60.0f,
            66.0f
        };
        const Rectangle rightArrow{
            centerX + cardWidth / 2.0f + gap + sideCardWidth + 16.0f,
            cardY + 38.0f,
            60.0f,
            66.0f
        };
        UiButton::DrawTextureFit(ui.arrowLeft, leftArrow);
        UiButton::DrawTextureFit(ui.arrowRight, rightArrow);

        for (int offset = -1; offset <= 1; ++offset) {
            const int levelIndex = (m_selectedLevel + offset + OfficialLevelCount) % OfficialLevelCount;
            const float width = offset == 0 ? cardWidth : sideCardWidth;
            const float height = offset == 0 ? 120.0f : 82.0f;
            const float cardCenterX = centerX + offset * (cardWidth / 2.0f + sideCardWidth / 2.0f + gap);
            const bool unlocked = levelIndex < m_context->unlockedLevel;
            const Color cardColor = unlocked ? (offset == 0 ? SKYBLUE : Fade(SKYBLUE, 0.65f)) : GRAY;

            Rectangle card{
                cardCenterX - width / 2.0f,
                cardY + (120.0f - height) / 2.0f,
                width,
                height
            };

        UiButton::DrawTextured(
            card,
            ui.menuLevelTitles[levelIndex],
            unlocked,
            "Crear nivel"
        );

            // const int levelNumber = OfficialLevels[levelIndex].levelIndex;
            // Texture2D levelTitleTexture = m_context->textureLibrary->GetLevelTitleTexture(levelNumber);
            //
            // const int fontSize = offset == 0 ? 24 : 18;
            // DrawRectangleRec(card, cardColor);
            // DrawRectangleLinesEx(card, offset == 0 ? 3.0f : 2.0f, DARKGRAY);
            // DrawTextFont(
            //     levelLabel,
            //     static_cast<int>(card.x + (card.width - MeasureText(levelLabel, fontSize)) / 2),
            //     static_cast<int>(card.y + (card.height - fontSize) / 2),
            //     fontSize,
            //     BLACK
            // );
        }
    } else if (m_editingLevels) {
        const char* title = "Editor de niveles";
        DrawTextFont(title, (m_context->windowWidth - MeasureText(title, 30)) / 2, 38, 30, BLACK);
        UiButton::DrawTextured(
            Rectangle{
                (m_context->windowWidth - 210.0f) / 2.0f,
                92.0f,
                210.0f,
                70.0f,
            },
            ui.menuCreateLevel,
            true,
            "Crear nivel"
        );

        const float listTop = 182.0f;
        const float listBottom = static_cast<float>(m_context->windowHeight) - 76.0f;

        const float rowHeight = 75.0f;
        const float rowWidth = std::min(820.0f, m_context->windowWidth - 40.0f);
        const float rowX = (m_context->windowWidth - rowWidth) / 2.0f;
        const float yPadding = 15.0f;
        bool drewLevel = false;

        for (std::size_t i = 0; i < m_context->customLevels->GetLevels().size(); ++i) {
            const float rowY = listTop + i * (rowHeight + yPadding) - m_customListScroll;
            if (rowY + rowHeight < listTop || rowY > listBottom) continue;
            drewLevel = true;
            const Rectangle row{rowX, rowY, rowWidth, rowHeight};
            DrawRectangleRec(row, BLACK);
            DrawRectangleLinesEx(row, 5.0f, {238, 18, 121, 255});
            const std::string& name = m_context->customLevels->GetLevels()[i].name;
            DrawTextFont(name.c_str(), static_cast<int>(row.x + 20), static_cast<int>(row.y + 20), 32, {252, 169, 33, 255});
            const bool approved = m_context->customLevels->HasValidApproval(i);
            // here:
            UiButton::DrawTextured(
                Rectangle{rowX + rowWidth - 260.0f, rowY + 9.0f, 120.0f, 57.0f},
                ui.menuEditAndPlay,
                true,
                "Editar y jugar"
            );
            UiButton::DrawTextured(
                Rectangle{rowX + rowWidth - 398.0f, rowY + 9.0f, 120.0f, 57.0f},
                ui.menuPublish,
                approved,
                approved ? "Publicar" : "Falta victoria"
            );
            UiButton::DrawTextured(
                Rectangle{rowX + rowWidth - 132.0f, rowY + 9.0f, 120.0f, 57.0f},
                ui.menuDelete,
                true,
                "Borrar"
            );
        }
        const PublicationSnapshot& publication =
            m_context->publications.GetSnapshot();
        if (!publication.message.empty()) {
            DrawTextFont(
                publication.message.c_str(),
                20,
                140,
                13,
                publication.status == PublicationStatus::Error ||
                    publication.status == PublicationStatus::Unavailable
                    ? MAROON
                    : DARKGREEN
            );
        }
        if (m_context->customLevels->GetLevels().empty()) {
            const char* emptyText = "Todavía no hay niveles personalizados";
            DrawTextFont(
                emptyText,
                (m_context->windowWidth - MeasureText(emptyText, 18)) / 2,
                170,
                18,
                DARKGRAY
            );
        } else if (!drewLevel) {
            const char* scrollText = "Usa la rueda del mouse para ver más niveles";
            DrawTextFont(scrollText, (m_context->windowWidth - MeasureText(scrollText, 16)) / 2, 170, 16, DARKGRAY);
        }

        UiButton::DrawTextured(
            UiButton::CenteredAtBottom(
                m_context->windowWidth,
                m_context->windowHeight,
                210.0f,
                70.0f,
                22.0f
            ),
            ui.menuBack,
            true,
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
            DrawTextFont(prompt, static_cast<int>(dialog.x + 24), static_cast<int>(dialog.y + 22), 20, BLACK);
            const Rectangle input{dialog.x + 24, dialog.y + 62, dialog.width - 48, 40};
            DrawRectangleRec(input, WHITE);
            DrawRectangleLinesEx(input, 1.0f, DARKGRAY);
            const char* placeholder = "Escribe un nombre...";
            const char* shownName = m_newLevelName.empty() ? placeholder : m_newLevelName.c_str();
            DrawTextFont(shownName, static_cast<int>(input.x + 10), static_cast<int>(input.y + 11), 18, DARKGRAY);
            DrawTextFont(
                TextFormat("%d/%d", static_cast<int>(CountCharacters(m_newLevelName)), MaxLevelNameCharacters),
                static_cast<int>(input.x + input.width - 58),
                static_cast<int>(input.y + 45),
                14,
                DARKGRAY
            );
            UiButton::DrawTextured(
                Rectangle{dialog.x + 24, dialog.y + 148, 130, 38},
                ui.menuDialogCancel,
                true,
                "Cancelar"
            );
            UiButton::DrawTextured(
                Rectangle{dialog.x + dialog.width - 154, dialog.y + 148, 130, 38},
                ui.menuDialogCreate,
                !m_newLevelName.empty(),
                "Crear"
            );
        }
    }

#ifndef __EMSCRIPTEN__
    DrawTextFont("Modo local: sin reserva de nombre ni publicacion online", 14, m_context->windowHeight - 18, 12, DARKGRAY);
#endif

    EndDrawing();
}
