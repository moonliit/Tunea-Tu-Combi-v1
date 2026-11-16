#include "app/MainMenuScene.hpp"
#include "app/AudioController.hpp"
#include "app/LevelCatalog.hpp"
#include "app/CustomLevelCatalog.hpp"
#include "app/SceneManager.hpp"
#include "app/game/Level.hpp"
#include "app/game/Part.hpp"
#include "app/game/PartDefinition.hpp"
#include "math/Terrain.hpp"
#include "physics/engine/World.hpp"
#include "rendering/Background.hpp"
#include "ui/UiButton.hpp"
#include <raylib.h>
#include <algorithm>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <memory>
#include <numbers>
#include <optional>
#include <utility>
#include <vector>

namespace {

float MainMenuTitleLevelTimer = 0.1f;
constexpr float TitleTerrainTileLength = 1600.0f;
constexpr float TitleTerrainBaseY = 260.0f;
constexpr int MaxLevelNameCharacters = 24;
constexpr float LevelCardHeight = 120.0f;
constexpr float LevelArrowHeight = 66.0f;

float LevelArrowY(float cardY) {
    return cardY + (LevelCardHeight - LevelArrowHeight) / 2.0f;
}

struct MainMenuButtons {
    Rectangle play;
    Rectangle community;
    Rectangle myLevels;
};

MainMenuButtons GetMainMenuButtons(int width, int height) {
    const float centerX = width / 2.0f;
    const float playY = height / 2.0f + 20.0f;
    return {
        Rectangle{centerX - 95.0f, playY, 190.0f, 190.0f},
        Rectangle{centerX - 286.0f, playY + 22.0f, 155.0f, 155.0f},
        Rectangle{centerX + 131.0f, playY + 22.0f, 155.0f, 155.0f}
    };
}

Rectangle CommunityRow(int width, int index) {
    return Rectangle{
        (width - 820.0f) / 2.0f,
        152.0f + index * 82.0f,
        820.0f,
        72.0f
    };
}

Rectangle CommunityPageButton(int width, int height, bool next) {
    return Rectangle{
        width / 2.0f + (next ? 15.0f : -145.0f),
        static_cast<float>(height - 147),
        130.0f,
        38.0f
    };
}

void DrawMenuChoiceLabel(const char* label, Rectangle button, int fontSize) {
    const float centerX = button.x + button.width / 2.0f;
    const float textWidth = MeasureTextFont(label, fontSize);
    const int x = static_cast<int>(centerX - textWidth / 2.0f);
    const int y = static_cast<int>(button.y + button.height - fontSize - 13.0f);
    DrawTextFont(label, x + 1, y + 1, fontSize, BLACK);
    DrawTextFont(label, x, y, fontSize, WHITE);
}

void DrawSelectorTitle(const char* title, int windowWidth, float top) {
    constexpr int fontSize = 30;
    const float textWidth = MeasureTextFont(title, fontSize);
    const Rectangle panel{
        (windowWidth - textWidth - 60.0f) / 2.0f,
        top,
        textWidth + 60.0f,
        58.0f
    };
    DrawRectangleRec(panel, Fade(BLACK, 0.88f));
    DrawRectangleLinesEx(panel, 3.0f, Color{129, 195, 71, 255});
    DrawRectangle(static_cast<int>(panel.x + 3.0f),
        static_cast<int>(panel.y + panel.height - 6.0f),
        static_cast<int>(panel.width - 6.0f), 3,
        Color{237, 18, 121, 255});
    DrawTextFont(title,
        static_cast<int>(panel.x + 30.0f),
        static_cast<int>(panel.y + 11.0f),
        fontSize, GOLD);
}

struct CreateLevelDialogLayout {
    Rectangle dialog;
    Rectangle cancel;
    Rectangle create;
};

CreateLevelDialogLayout GetCreateLevelDialogLayout(int width, int height) {
    const Rectangle dialog{
        (width - 500.0f) / 2.0f,
        (height - 210.0f) / 2.0f,
        500.0f,
        210.0f
    };
    return {
        dialog,
        Rectangle{dialog.x + 24.0f, dialog.y + 148.0f, 130.0f, 38.0f},
        Rectangle{dialog.x + dialog.width - 154.0f,
            dialog.y + 148.0f, 130.0f, 38.0f}
    };
}

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
    m_selectedLevel(std::clamp(context->unlockedLevel - 1, 0, OfficialLevelCount - 1))
{
    InitTitleWorld();
}


void MainMenuScene::Update(float deltaTime) {
    if (auto copy = m_context->publications.TakeCopyRequest()) {
        if (m_context->customLevels->AddCopy(copy.value())) {
            m_selectingLevel = true;
            m_editingLevels = true;
            m_browsingCommunity = false;
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

    if (MainMenuTitleLevelTimer >= 0.0f) {
        MainMenuTitleLevelTimer -= deltaTime;
    } else {
        UpdateTitleWorld(deltaTime);
    }
}

void MainMenuScene::ProcessInput(float) {
    if (m_context->returnedToSelector) {
        m_context->returnedToSelector = false;
        m_selectedLevel = std::clamp(m_context->unlockedLevel - 1, 0, OfficialLevelCount - 1);
        if (m_context->requestedCommunitySelector) {
            m_context->requestedCommunitySelector = false;
            m_context->requestedCustomSelector.reset();
            m_selectingLevel = false;
            m_editingLevels = false;
            m_browsingCommunity = true;
            m_communityScroll = 0;
            m_context->publications.OpenCatalog();
        } else if (m_context->requestedCustomSelector.has_value()) {
            m_selectingLevel = true;
            m_editingLevels = *m_context->requestedCustomSelector;
            m_browsingCommunity = false;
            m_context->requestedCustomSelector.reset();
        }
    }

    if (m_context->returnToCustomDesign) {
        m_context->returnToCustomDesign = false;
        m_context->sceneRequester->RequestSceneChange({SceneId::CustomLevelEditor});
        return;
    }

    if ((m_selectingLevel || m_editingLevels || m_browsingCommunity) && !m_creatingLevel &&
        IsKeyPressed(KEY_ESCAPE)) {
        m_context->audioController->PlayUiClick();
        m_context->audioController->ResetToMenu();
        m_selectingLevel = false;
        m_editingLevels = false;
        m_browsingCommunity = false;
        m_selectedLevel = 0;
        return;
    }

    if (m_editingLevels) {
        ProcessLevelEditorInput();
        return;
    }
    if (m_browsingCommunity) {
        ProcessCommunityInput();
        return;
    }

    Rectangle menuButton{
        static_cast<float>(m_context->windowWidth - 230),
        20.0f,
        210.0f,
        69.0f
    };
    if (m_selectingLevel && UiButton::WasClicked(menuButton)) {
        m_context->audioController->PlayUiClick();
        m_context->audioController->ResetToMenu();
        m_selectingLevel = false;
        m_selectedLevel = 0;
        return;
    }

    const MainMenuButtons buttons = GetMainMenuButtons(
        m_context->windowWidth, m_context->windowHeight
    );
    if (!m_selectingLevel && UiButton::WasClicked(buttons.play)) {
        m_context->audioController->PlayUiClick();
        m_context->audioController->SetCurrentScenario({Scenario::LevelSelect});
        m_selectingLevel = true;
        m_editingLevels = false;
        return;
    }
    if (!m_selectingLevel && UiButton::WasClicked(buttons.community)) {
        m_context->audioController->PlayUiClick();
        m_context->audioController->SetCurrentScenario({Scenario::LevelSelect});
        m_browsingCommunity = true;
        m_communityScroll = 0;
        m_context->publications.OpenCatalog();
        return;
    }
    if (!m_selectingLevel && UiButton::WasClicked(buttons.myLevels)) {
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
        LevelArrowY(cardY),
        60.0f,
        LevelArrowHeight
    };
    const Rectangle rightArrow{
        centerX + cardWidth / 2.0f + gap + sideCardWidth + 16.0f,
        LevelArrowY(cardY),
        60.0f,
        LevelArrowHeight
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

void MainMenuScene::ProcessCommunityInput() {
    const int width = m_context->windowWidth;
    const int height = m_context->windowHeight;
    const PublicCatalogSnapshot& catalog =
        m_context->publications.GetCatalogSnapshot();

    if (UiButton::WasClicked(UiButton::CenteredAtBottom(
            width, height, 210.0f, 70.0f, 22.0f))) {
        m_context->audioController->PlayUiClick();
        m_context->audioController->ResetToMenu();
        m_browsingCommunity = false;
        return;
    }

    if (!catalog.loading && catalog.hasPrevious &&
        UiButton::WasClicked(CommunityPageButton(width, height, false))) {
        m_context->audioController->PlayUiClick();
        m_communityScroll = 0;
        m_context->publications.PreviousCatalogPage();
        return;
    }
    if (!catalog.loading && catalog.hasNext &&
        UiButton::WasClicked(CommunityPageButton(width, height, true))) {
        m_context->audioController->PlayUiClick();
        m_communityScroll = 0;
        m_context->publications.NextCatalogPage();
        return;
    }

    const int maxScroll = std::max(0,
        static_cast<int>(catalog.levels.size()) - 5);
    m_communityScroll = std::clamp(
        m_communityScroll - static_cast<int>(GetMouseWheelMove()),
        0, maxScroll
    );
    for (int row = 0; row < 5; ++row) {
        const int index = m_communityScroll + row;
        if (index >= static_cast<int>(catalog.levels.size())) break;
        if (UiButton::WasClicked(CommunityRow(width, row))) {
            m_context->audioController->PlayUiClick();
            m_context->publications.PlayPublicLevel(catalog.levels[index].id);
            return;
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

        const CreateLevelDialogLayout dialog = GetCreateLevelDialogLayout(
            m_context->windowWidth, m_context->windowHeight
        );
        if (UiButton::WasClicked(dialog.cancel)) {
            m_creatingLevel = false;
            m_newLevelName.clear();
        } else if (UiButton::WasClicked(dialog.create) && !m_newLevelName.empty()) {
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
    RenderTitleWorld();

    if (!m_selectingLevel && !m_browsingCommunity) {
        DrawRectangle(0, 0, m_context->windowWidth, m_context->windowHeight, Fade(BLACK, 0.12f));
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

        const MainMenuButtons buttons = GetMainMenuButtons(
            m_context->windowWidth, m_context->windowHeight
        );
        UiButton::DrawTextured(
            buttons.play,
            ui.game.modePlay,
            true,
            "Jugar"
        );
        UiButton::DrawTextured(
            buttons.community,
            ui.menuCommunity,
            true,
            "Comunidad"
        );
        UiButton::DrawTextured(
            buttons.myLevels,
            ui.menuMyLevels,
            true,
            "Mis niveles"
        );
        DrawMenuChoiceLabel("COMUNIDAD", buttons.community, 17);
        DrawMenuChoiceLabel("JUGAR", buttons.play, 21);
        DrawMenuChoiceLabel("MIS NIVELES", buttons.myLevels, 17);
    } else if (m_browsingCommunity) {
        DrawRectangle(0, 0, m_context->windowWidth,
            m_context->windowHeight, Fade(BLACK, 0.22f));
        DrawSelectorTitle("NIVELES DE LA COMUNIDAD",
            m_context->windowWidth, 24.0f);
        const PublicCatalogSnapshot& catalog =
            m_context->publications.GetCatalogSnapshot();
        if (catalog.loading && catalog.levels.empty()) {
            DrawTextFont("Cargando niveles...", 200, 166, 22, WHITE);
        } else if (!catalog.error.empty()) {
            DrawTextFont(catalog.error.c_str(), 200, 166, 20, WHITE);
        } else if (catalog.levels.empty()) {
            DrawTextFont("Todavía no hay niveles publicados.", 200, 166, 20, WHITE);
        }
        for (int row = 0; row < 5; ++row) {
            const int index = m_communityScroll + row;
            if (index >= static_cast<int>(catalog.levels.size())) break;
            const PublicLevelSummary& level = catalog.levels[index];
            const Rectangle bounds = CommunityRow(m_context->windowWidth, row);
            DrawRectangleRec(bounds, Fade(BLACK, 0.86f));
            DrawRectangleLinesEx(bounds, 2.0f, Color{237, 18, 121, 255});
            DrawTextFont(level.name.c_str(), static_cast<int>(bounds.x + 18),
                static_cast<int>(bounds.y + 9), 23, WHITE);
            const std::string author = "Por " + level.author;
            DrawTextFont(author.c_str(), static_cast<int>(bounds.x + 18),
                static_cast<int>(bounds.y + 43), 14, SKYBLUE);
            UiButton::Draw(Rectangle{bounds.x + bounds.width - 123.0f,
                bounds.y + 13.0f, 105.0f, 46.0f},
                "Jugar", Color{129, 195, 71, 255}, BLACK, 19);
        }
        if (catalog.levels.size() > 5) {
            DrawTextFont("Usa la rueda para ver más niveles",
                28, m_context->windowHeight - 165, 13, WHITE);
        }
        UiButton::Draw(CommunityPageButton(m_context->windowWidth,
                m_context->windowHeight, false),
            "Anterior", catalog.hasPrevious ? BLACK : DARKGRAY,
            WHITE, 16);
        UiButton::Draw(CommunityPageButton(m_context->windowWidth,
                m_context->windowHeight, true),
            "Siguiente", catalog.hasNext ? BLACK : DARKGRAY,
            WHITE, 16);
        UiButton::DrawTextured(UiButton::CenteredAtBottom(
                m_context->windowWidth, m_context->windowHeight,
                210.0f, 70.0f, 22.0f),
            ui.menuBack, true, "Menú");
    } else if (!m_editingLevels) {
        DrawSelectorTitle("SELECCIONA UN NIVEL",
            m_context->windowWidth,
            m_context->windowHeight / 2.0f - 145.0f);
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
            "Menú"
        );

        const float cardWidth = std::min(200.0f, (m_context->windowWidth - 180.0f) / 3.0f);
        const float sideCardWidth = cardWidth * 0.68f;
        const float gap = 20.0f;
        const float centerX = m_context->windowWidth / 2.0f;
        const float cardY = static_cast<float>(m_context->windowHeight) / 2.0f - 60.0f;

        const Rectangle leftArrow{
            centerX - cardWidth / 2.0f - gap - sideCardWidth - 72.0f,
            LevelArrowY(cardY),
            60.0f,
            LevelArrowHeight
        };
        const Rectangle rightArrow{
            centerX + cardWidth / 2.0f + gap + sideCardWidth + 16.0f,
            LevelArrowY(cardY),
            60.0f,
            LevelArrowHeight
        };
        UiButton::DrawTextureFit(ui.arrowLeft, leftArrow);
        UiButton::DrawTextureFit(ui.arrowRight, rightArrow);

        for (int offset = -1; offset <= 1; ++offset) {
            const int levelIndex = (m_selectedLevel + offset + OfficialLevelCount) % OfficialLevelCount;
            const float width = offset == 0 ? cardWidth : sideCardWidth;
            const float height = offset == 0 ? LevelCardHeight : 82.0f;
            const float cardCenterX = centerX + offset * (cardWidth / 2.0f + sideCardWidth / 2.0f + gap);
            const bool unlocked = levelIndex < m_context->unlockedLevel;
            const Color cardColor = unlocked ? (offset == 0 ? SKYBLUE : Fade(SKYBLUE, 0.65f)) : GRAY;

            Rectangle card{
                cardCenterX - width / 2.0f,
                cardY + (LevelCardHeight - height) / 2.0f,
                width,
                height
            };
            UiButton::DrawTextured(
                card,
                ui.menuLevelTitles[levelIndex],
                unlocked,
                TextFormat("Nivel %d", levelIndex + 1)
            );
        }
    } else if (m_editingLevels) {
        DrawSelectorTitle("MIS NIVELES", m_context->windowWidth, 24.0f);
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
                static_cast<int>((m_context->windowWidth - MeasureTextFont(emptyText, 18)) / 2),
                170,
                18,
                DARKGRAY
            );
        } else if (!drewLevel) {
            const char* scrollText = "Usa la rueda del mouse para ver más niveles";
            DrawTextFont(
                scrollText,
                static_cast<int>((m_context->windowWidth - MeasureTextFont(scrollText, 16)) / 2),
                170,
                16,
                DARKGRAY
            );
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
            const CreateLevelDialogLayout layout = GetCreateLevelDialogLayout(
                m_context->windowWidth, m_context->windowHeight
            );
            const Rectangle& dialog = layout.dialog;
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
                layout.cancel,
                ui.menuDialogCancel,
                true,
                "Cancelar"
            );
            UiButton::DrawTextured(
                layout.create,
                ui.menuDialogCreate,
                !m_newLevelName.empty(),
                "Crear"
            );
        }
    }

#ifndef __EMSCRIPTEN__
    const char* localModeText =
        "Modo local: sin reserva de nombre ni publicación en línea";
    const Rectangle localModeBadge{
        8.0f,
        static_cast<float>(m_context->windowHeight - 31),
        MeasureTextFont(localModeText, 14) + 12.0f,
        27.0f
    };
    DrawRectangleRec(localModeBadge, Fade(BLACK, 0.78f));
    DrawRectangleLinesEx(localModeBadge, 1.0f, Color{237, 18, 121, 255});
    DrawTextFont(localModeText, 14, m_context->windowHeight - 25, 14, WHITE);
#endif

    EndDrawing();
}

void MainMenuScene::InitTitleWorld() {
    ResetTitleWorld();
}

void MainMenuScene::ResetTitleWorld() {
    const float tileLength = TitleTerrainTileLength;
    const float baseY = TitleTerrainBaseY;
    const float depth = 500.0f;

    m_titleTerrainLength = tileLength * 3.0f;
    // spawn in middle tile to avoid a visible beginning
    m_titleModel = std::make_unique<BuildModel>(
        Math::GridCoord{6, 4},
        Math::Vector2D{220.0f + tileLength, baseY + 20.0f},
        40.0f
    );

    // sample vehicle
    m_titleModel->PlacePart(Math::GridCoord{2, 1}, PartType::SquareBlock);
    m_titleModel->PlacePart(Math::GridCoord{3, 1}, PartType::SquareBlock);
    m_titleModel->PlacePart(Math::GridCoord{4, 1}, PartType::DriverBox);
    m_titleModel->PlacePart(Math::GridCoord{1, 1}, PartType::Fan);
    m_titleModel->PlacePart(Math::GridCoord{2, 2}, PartType::CircleWheel);
    m_titleModel->PlacePart(Math::GridCoord{4, 2}, PartType::CircleWheel);

    // periodic tileable terrain for seamless looping
    std::vector<Math::Vector2D> top;
    const float step = 40.0f;
    for (float x = -400.0f; x <= m_titleTerrainLength + 400.0f; x += step) {
        const float phase = (2.0f * std::numbers::pi_v<float> * x) / tileLength;
        const float y = baseY
            + 18.0f * std::sin(phase)
            + 10.0f * std::sin(2.0f * phase + 1.3f);
        top.push_back({x, y});
    }

    Math::TerrainDefinition terrain;
    Math::TerrainPolygon polygon;
    Math::TerrainSegment segment;
    segment.type = Math::TerrainSegmentType::Line;
    segment.points = top;
    segment.points.push_back({top.back().x, baseY + depth});
    segment.points.push_back({top.front().x, baseY + depth});
    segment.points.push_back(top.front());
    polygon.segments.push_back(std::move(segment));
    terrain.polygons.push_back(std::move(polygon));

    m_titleSession = std::make_unique<PlaySession>(*m_titleModel, terrain, Math::Vector2D{0.0f, 392.0f});
    m_titleSession->Start();

    m_titleRenderer = std::make_unique<WorldRenderer>(*m_context->textureLibrary, BackgroundId::Barranco);

    m_titleCamera = {};
    m_titleCamera.offset = {m_context->windowWidth * 0.45f, m_context->windowHeight * 0.62f};
    m_titleCamera.rotation = 0.0f;
    m_titleCamera.zoom = 0.8f;
    m_titleCamera.target = {tileLength + 220.0f, baseY - 40.0f};
    if (auto follow = GetTitleDriverPosition()) {
        m_titleCamera.target = {follow->x, follow->y};
    }

    for (const Part* constPart : m_titleSession->GetRuntimeGrid().GetActiveParts()) {
        Part* part = const_cast<Part*>(constPart);
        if (part->GetType() == PartType::Fan) {
            part->Activate();
        }
    }

    m_titleRenderer->SetBackgroundOrigin(m_titleCamera.target.x);
    m_titleElapsedMs = 0.0;
}

void MainMenuScene::UpdateTitleWorld(float deltaTime) {
    if (!m_titleSession) InitTitleWorld();
    m_titleSession->Update(deltaTime);
    m_titleElapsedMs += static_cast<double>(deltaTime) * 1000.0;

    auto followOpt = GetTitleDriverPosition();
    if (followOpt && std::isfinite(followOpt->x) && std::isfinite(followOpt->y)) {
        Vector2 follow = *followOpt;

        // wrap vehicle within middle tile for seamless looping
        const float tileLength = TitleTerrainTileLength;
        auto* world = dynamic_cast<Physics::World*>(&m_titleSession->GetPhysics());
        if (world && follow.x > tileLength * 2.0f) {
            for (auto& body : world->GetBodies()) {
                if (!body || body->GetBodyType() == BodyType::Static) continue;
                body->GetTransformMut().position.x -= tileLength;
                body->UpdateAABB();
            }
            follow.x -= tileLength;
        } else if (world && follow.x < tileLength) {
            for (auto& body : world->GetBodies()) {
                if (!body || body->GetBodyType() == BodyType::Static) continue;
                body->GetTransformMut().position.x += tileLength;
                body->UpdateAABB();
            }
            follow.x += tileLength;
        }

        m_titleCamera.target.x = follow.x - 120.0f;
        m_titleCamera.target.y = follow.y - 20.0f;
        if (m_titleRenderer) {
            m_titleRenderer->SetBackgroundOrigin(m_titleCamera.target.x);
        }
    }
}

void MainMenuScene::RenderTitleWorld() const {
    if (!m_titleRenderer || !m_titleSession || !m_titleModel) return;
    m_titleRenderer->Render(
        m_titleCamera,
        GameState::Play,
        m_titleModel->GetGrid(),
        *m_titleSession,
        std::nullopt,
        PartOrientation::Up,
        std::nullopt,
        PartOrientation::Up,
        m_context->windowHeight,
        {},
        !m_selectingLevel && !m_browsingCommunity
    );
}

std::optional<Vector2> MainMenuScene::GetTitleDriverPosition() const {
    if (!m_titleSession) return std::nullopt;

    const EditorGrid& grid = m_titleSession->GetRuntimeGrid();
    const auto finitePos = [](const PartRenderState& state) -> std::optional<Vector2> {
        const float x = static_cast<float>(state.position.x);
        const float y = static_cast<float>(state.position.y);
        if (std::isfinite(x) && std::isfinite(y)) return Vector2{x, y};
        return std::nullopt;
    };

    // prefer driver box position
    for (const Part* part : grid.GetActiveParts()) {
        if (part->GetType() == PartType::DriverBox &&
            part->HasContainedPart() && part->GetContainedPart() &&
            part->GetContainedPart()->GetType() == PartType::Driver) {
            if (auto pos = finitePos(part->GetRenderState(grid))) {
                return pos;
            }
        }
    }

    for (const Part* part : grid.GetActiveParts()) {
        if (part->GetType() == PartType::Driver) continue;
        if (auto pos = finitePos(part->GetRenderState(grid))) {
            return pos;
        }
    }

    return std::nullopt;
}
