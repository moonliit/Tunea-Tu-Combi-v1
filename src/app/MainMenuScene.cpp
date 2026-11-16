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
#include <cctype>
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
constexpr float CustomListTop = 220.0f;
constexpr float CustomRowHeight = 75.0f;
constexpr float CustomRowGap = 15.0f;

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
        160.0f + index * 82.0f,
        820.0f,
        72.0f
    };
}

// Buttons of a row, counted from the right: 0 is "Jugar", 1 "Eliminar" and 2 "Copiar"
// (the last two only show up on your own levels).
Rectangle CommunityRowButton(int width, int row, int fromRight) {
    const Rectangle bounds = CommunityRow(width, row);
    return Rectangle{
        bounds.x + bounds.width - 123.0f - fromRight * 115.0f,
        bounds.y + 13.0f,
        105.0f,
        46.0f
    };
}

// The search and "propios" buttons sit side by side, centered between the page arrows.
Rectangle CommunityMineButton(int width, int height) {
    return Rectangle{width / 2.0f - 11.0f,
        static_cast<float>(height - 158), 96.0f, 60.0f};
}

Rectangle CommunityPageButton(int width, int height, bool next) {
    return Rectangle{
        width / 2.0f + (next ? 132.0f : -228.0f),
        static_cast<float>(height - 158),
        96.0f,
        80.0f
    };
}

Rectangle CommunitySearchButton(int width, int height) {
    return Rectangle{width / 2.0f - 85.0f,
        static_cast<float>(height - 158), 62.0f, 60.0f};
}

struct CommunitySearchDialogLayout {
    Rectangle dialog;
    Rectangle input;
    Rectangle close;
};

CommunitySearchDialogLayout GetCommunitySearchDialogLayout(int width, int height) {
    const Rectangle dialog{(width - 520.0f) / 2.0f,
        (height - 244.0f) / 2.0f, 520.0f, 244.0f};
    return {
        dialog,
        Rectangle{dialog.x + 24.0f, dialog.y + 105.0f,
            dialog.width - 48.0f, 58.0f},
        Rectangle{dialog.x + dialog.width - 58.0f,
            dialog.y + 16.0f, 38.0f, 38.0f}
    };
}

Rectangle CreateLevelButton(int width) {
    return Rectangle{(width - 72.0f) / 2.0f, 92.0f, 72.0f, 72.0f};
}

void DrawMenuChoiceLabel(const char* label, Rectangle button, int fontSize) {
    const float centerX = button.x + button.width / 2.0f;
    const float textWidth = MeasureTextFont(label, fontSize);
    const int x = static_cast<int>(centerX - textWidth / 2.0f);
    const int y = static_cast<int>(button.y + button.height - fontSize - 13.0f);
    DrawTextFont(label, x + 1, y + 1, fontSize, BLACK);
    DrawTextFont(label, x, y, fontSize, WHITE);
}

void DrawCenteredCommunityText(
    const char* text,
    int width,
    int y,
    int fontSize,
    Color color
) {
    DrawTextFont(text,
        static_cast<int>((width - MeasureTextFont(text, fontSize)) / 2.0f),
        y, fontSize, color);
}

void DrawCommunityNotice(
    const char* text,
    int width,
    int y,
    int fontSize,
    Color color
) {
    const float textWidth = MeasureTextFont(text, fontSize);
    const Rectangle badge{
        (width - textWidth - 32.0f) / 2.0f,
        static_cast<float>(y - 7),
        textWidth + 32.0f,
        static_cast<float>(fontSize + 14)
    };
    DrawRectangleRec(badge, Fade(BLACK, 0.78f));
    DrawRectangleLinesEx(badge, 1.0f, Color{237, 18, 121, 255});
    DrawTextFont(text,
        static_cast<int>(badge.x + 16.0f), y, fontSize, color);
}

void DrawCommunityPageButton(
    Rectangle bounds,
    const Texture2D& arrow,
    const Texture2D& label,
    bool enabled
) {
    const Rectangle icon{
        bounds.x + (bounds.width - 58.0f) / 2.0f,
        bounds.y, 58.0f, 58.0f
    };
    UiButton::DrawTextureFit(arrow, icon,
        enabled ? WHITE : Color{255, 255, 255, 105});
    const float labelHeight = 26.0f;
    const float labelWidth = labelHeight * label.width / label.height;
    UiButton::DrawTextureFit(label,
        Rectangle{bounds.x + (bounds.width - labelWidth) / 2.0f,
            bounds.y + 61.0f, labelWidth, labelHeight},
        enabled ? WHITE : Color{255, 255, 255, 105});
}

void DrawCommunityMineButton(Rectangle bounds, bool active) {
    const bool hovered = UiButton::IsHovered(bounds);
    DrawRectangleRec(bounds,
        hovered || active ? Color{55, 59, 60, 255} : BLACK);
    DrawRectangleLinesEx(bounds, 3.0f,
        active ? GOLD : Color{129, 195, 71, 255});
    const char* label = "PROPIOS";
    DrawTextFont(label,
        static_cast<int>(bounds.x + (bounds.width - MeasureTextFont(label, 18)) / 2.0f),
        static_cast<int>(bounds.y + 21.0f), 18, active ? GOLD : WHITE);
}

void DrawCommunitySearchButton(Rectangle bounds, bool active) {
    const bool hovered = UiButton::IsHovered(bounds);
    DrawRectangleRec(bounds,
        hovered || active ? Color{55, 59, 60, 255} : BLACK);
    DrawRectangleLinesEx(bounds, 3.0f,
        active ? GOLD : Color{129, 195, 71, 255});
    const Vector2 center{bounds.x + 26.0f, bounds.y + 25.0f};
    DrawCircleLines(static_cast<int>(center.x),
        static_cast<int>(center.y), 12.0f, GOLD);
    DrawLineEx({center.x + 9.0f, center.y + 9.0f},
        {center.x + 23.0f, center.y + 23.0f}, 5.0f, GOLD);
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
        (width - 520.0f) / 2.0f,
        (height - 302.0f) / 2.0f,
        520.0f,
        302.0f
    };
    return {
        dialog,
        Rectangle{dialog.x + 24.0f, dialog.y + 208.0f, 186.0f, 80.0f},
        Rectangle{dialog.x + dialog.width - 210.0f,
            dialog.y + 208.0f, 186.0f, 80.0f}
    };
}

void DrawDialogAction(
    Rectangle bounds,
    const UiButtonSkin& skin,
    const char* label,
    bool enabled
) {
    const bool hovered = enabled && UiButton::IsHovered(bounds);
    const bool pressed = hovered && IsMouseButtonDown(MOUSE_BUTTON_LEFT);
    UiButton::DrawTextureFit(
        UiButton::ResolveButtonTexture(skin, enabled, hovered, pressed),
        Rectangle{bounds.x, bounds.y, bounds.width, 54.0f}
    );
    DrawTextFont(label,
        static_cast<int>(bounds.x +
            (bounds.width - MeasureTextFont(label, 16)) / 2.0f),
        static_cast<int>(bounds.y + 62.0f), 16,
        enabled ? (hovered ? GOLD : WHITE) : Color{190, 201, 203, 255});
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
    context->focusCustomEditorCamera = true;
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
            m_communityQuery.clear();
            m_communitySearchOpen = false;
            m_context->publications.OpenCatalog();
        } else if (m_context->requestedCustomSelector.has_value()) {
            m_selectingLevel = true;
            m_editingLevels = *m_context->requestedCustomSelector;
            m_browsingCommunity = false;
            m_context->requestedCustomSelector.reset();
        }
    }

    if (m_browsingCommunity && m_communitySearchOpen &&
        IsKeyPressed(KEY_ESCAPE)) {
        m_communitySearchOpen = false;
        return;
    }

    if ((m_selectingLevel || m_editingLevels || m_browsingCommunity) && !m_creatingLevel &&
        IsKeyPressed(KEY_ESCAPE)) {
        m_context->audioController->PlayUiClick();
        m_context->audioController->ResetToMenu();
        m_selectingLevel = false;
        m_editingLevels = false;
        m_browsingCommunity = false;
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
        m_selectedLevel = std::clamp(m_context->unlockedLevel - 1, 0, OfficialLevelCount - 1);
        return;
    }
    if (!m_selectingLevel && UiButton::WasClicked(buttons.community)) {
        m_context->audioController->PlayUiClick();
        m_context->audioController->SetCurrentScenario({Scenario::LevelSelect});
        m_browsingCommunity = true;
        m_communityScroll = 0;
        m_communityQuery.clear();
        m_communitySearchOpen = false;
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
    // what the server remembers
    const int serverLevel = std::min(OfficialLevelCount, m_context->identity.GetUnlockedLevel());
    m_context->unlockedLevel = std::max(m_context->unlockedLevel, serverLevel);
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

    if (m_communitySearchOpen) {
        bool changed = false;
        int codepoint = GetCharPressed();
        while (codepoint > 0) {
            if (codepoint >= 32 && codepoint != 127 &&
                CountCharacters(m_communityQuery) < 40) {
                AppendCodepoint(m_communityQuery, codepoint);
                changed = true;
            }
            codepoint = GetCharPressed();
        }
        if (IsKeyPressed(KEY_BACKSPACE) && !m_communityQuery.empty()) {
            RemoveLastCharacter(m_communityQuery);
            changed = true;
        }
        if (changed) {
            m_communityScroll = 0;
            m_context->publications.SearchCatalog(m_communityQuery);
        }
        if (IsKeyPressed(KEY_ENTER) || UiButton::WasClicked(
                GetCommunitySearchDialogLayout(width, height).close)) {
            m_communitySearchOpen = false;
        }
        return;
    }

    if (UiButton::WasClicked(CommunitySearchButton(width, height))) {
        m_context->audioController->PlayUiClick();
        m_communitySearchOpen = true;
        return;
    }

    if (UiButton::WasClicked(UiButton::CenteredAtBottom(
            width, height, 210.0f, 70.0f, 22.0f))) {
        m_context->audioController->PlayUiClick();
        m_context->audioController->ResetToMenu();
        m_browsingCommunity = false;
        m_communitySearchOpen = false;
        return;
    }

    if (UiButton::WasClicked(CommunityMineButton(width, height))) {
        m_context->audioController->PlayUiClick();
        m_communityScroll = 0;
        m_context->publications.ShowOnlyMine(!catalog.onlyMine);
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
        const PublicLevelSummary& level = catalog.levels[index];
        if (level.mine && UiButton::WasClicked(CommunityRowButton(width, row, 1))) {
            m_context->audioController->PlayUiClick();
            m_context->publications.RetirePublicLevel(level.id);
            return;
        }
        if (level.mine && UiButton::WasClicked(CommunityRowButton(width, row, 2))) {
            m_context->audioController->PlayUiClick();
            m_context->publications.CopyPublicLevel(level.id);
            return;
        }
        if (UiButton::WasClicked(CommunityRow(width, row))) {
            m_context->audioController->PlayUiClick();
            m_context->publications.PlayPublicLevel(level.id);
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

    const Rectangle createButton = CreateLevelButton(m_context->windowWidth);
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
        return;
    }

    const float listTop = CustomListTop;
    const float listBottom = static_cast<float>(m_context->windowHeight) - 76.0f;
    const float rowHeight = CustomRowHeight;
    const float maxScroll = std::max(
        0.0f,
        m_context->customLevels->GetLevels().size() *
            (rowHeight + CustomRowGap) - CustomRowGap - (listBottom - listTop)
    );
    m_customListScroll = std::clamp(
        m_customListScroll - GetMouseWheelMove() * 40.0f,
        0.0f,
        maxScroll
    );

    const float yPadding = CustomRowGap;
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
            m_context->focusCustomEditorCamera = true;
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
    if (m_selectingLevel || m_browsingCommunity) {
        DrawRectangle(0, 0, m_context->windowWidth,
            m_context->windowHeight, Fade(BLACK, 0.22f));
    }

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
        const PublicCatalogSnapshot& catalog =
            m_context->publications.GetCatalogSnapshot();
        DrawSelectorTitle(catalog.onlyMine ? "MIS PUBLICACIONES"
                                           : "NIVELES DE LA COMUNIDAD",
            m_context->windowWidth, 24.0f);
        if (!m_communityQuery.empty()) {
            const std::string queryLabel = "Búsqueda: " + m_communityQuery;
            DrawCommunityNotice(queryLabel.c_str(),
                m_context->windowWidth, 96, 15, GOLD);
        }
        if (!catalog.error.empty()) {
            DrawCommunityNotice(catalog.error.c_str(),
                m_context->windowWidth, 166, 20, WHITE);
        } else if (catalog.loading) {
            DrawCommunityNotice(
                m_communityQuery.empty() ? "Cargando niveles..." : "Buscando...",
                m_context->windowWidth,
                catalog.levels.empty() ? 166 : 130,
                catalog.levels.empty() ? 22 : 16,
                WHITE);
        } else if (catalog.levels.empty()) {
            DrawCommunityNotice(
                !m_communityQuery.empty()
                    ? "No se encontraron niveles con ese nombre."
                    : catalog.onlyMine
                    ? "Todavía no has publicado niveles."
                    : "Todavía no hay niveles publicados.",
                m_context->windowWidth, 166, 20, WHITE);
        }
        const PublicationSnapshot& publication =
            m_context->publications.GetSnapshot();
        if (!publication.message.empty() && !catalog.loading && catalog.error.empty()) {
            DrawCommunityNotice(publication.message.c_str(),
                m_context->windowWidth, 130, 16,
                publication.status == PublicationStatus::Error
                    ? Color{255, 140, 140, 255}
                    : WHITE);
        }
        for (int row = 0; catalog.error.empty() && row < 5; ++row) {
            const int index = m_communityScroll + row;
            if (index >= static_cast<int>(catalog.levels.size())) break;
            const PublicLevelSummary& level = catalog.levels[index];
            const Rectangle bounds = CommunityRow(m_context->windowWidth, row);
            DrawRectangleRec(bounds, Fade(BLACK, 0.86f));
            DrawRectangleLinesEx(bounds, 2.0f, Color{237, 18, 121, 255});
            // A long name gets smaller instead of running under the buttons.
            const float nameSpace = CommunityRowButton(m_context->windowWidth, row,
                level.mine ? 2 : 0).x - bounds.x - 30.0f;
            int nameSize = 23;
            while (nameSize > 14 &&
                   MeasureTextFont(level.name.c_str(), nameSize) > nameSpace) {
                --nameSize;
            }
            DrawTextFont(level.name.c_str(), static_cast<int>(bounds.x + 18),
                static_cast<int>(bounds.y + 9), nameSize, WHITE);
            const std::string author = "Por " + level.author;
            DrawTextFont(author.c_str(), static_cast<int>(bounds.x + 18),
                static_cast<int>(bounds.y + 43), 14, SKYBLUE);
            if (level.mine) {
                UiButton::Draw(CommunityRowButton(m_context->windowWidth, row, 2),
                    "Copiar", Color{252, 169, 33, 255}, BLACK, 19);
                UiButton::Draw(CommunityRowButton(m_context->windowWidth, row, 1),
                    "Eliminar", Color{214, 69, 65, 255}, BLACK, 19);
            }
            UiButton::Draw(CommunityRowButton(m_context->windowWidth, row, 0),
                "Jugar", Color{129, 195, 71, 255}, BLACK, 19);
        }
        if (catalog.levels.size() > 5) {
            DrawTextFont("Usa la rueda para ver más niveles",
                28, m_context->windowHeight - 165, 13, WHITE);
        }
        DrawCommunityPageButton(
            CommunityPageButton(m_context->windowWidth,
                m_context->windowHeight, false),
            ui.arrowLeft, ui.communityPreviousLabel,
            catalog.hasPrevious && !catalog.loading);
        DrawCommunityPageButton(
            CommunityPageButton(m_context->windowWidth,
                m_context->windowHeight, true),
            ui.arrowRight, ui.communityNextLabel,
            catalog.hasNext && !catalog.loading);
        DrawCommunitySearchButton(
            CommunitySearchButton(m_context->windowWidth,
                m_context->windowHeight),
            m_communitySearchOpen || !m_communityQuery.empty());
        DrawCommunityMineButton(
            CommunityMineButton(m_context->windowWidth,
                m_context->windowHeight),
            catalog.onlyMine);
        UiButton::DrawTextured(UiButton::CenteredAtBottom(
                m_context->windowWidth, m_context->windowHeight,
                210.0f, 70.0f, 22.0f),
            ui.menuBack, true, "Menú");
        if (m_communitySearchOpen) {
            DrawRectangle(0, 0, m_context->windowWidth,
                m_context->windowHeight, Fade(BLACK, 0.34f));
            const CommunitySearchDialogLayout search =
                GetCommunitySearchDialogLayout(
                    m_context->windowWidth, m_context->windowHeight
                );
            const Color accentGreen{129, 195, 71, 255};
            const Color accentPink{237, 18, 121, 255};
            const Color mutedText{190, 201, 203, 255};
            DrawRectangleRec(Rectangle{
                search.dialog.x + 7.0f, search.dialog.y + 8.0f,
                search.dialog.width, search.dialog.height
            }, Fade(BLACK, 0.55f));
            DrawRectangleRec(search.dialog, Color{12, 17, 24, 250});
            DrawRectangleLinesEx(search.dialog, 3.0f, accentGreen);
            DrawRectangle(static_cast<int>(search.dialog.x + 3.0f),
                static_cast<int>(search.dialog.y + 84.0f),
                static_cast<int>(search.dialog.width - 6.0f), 3, accentPink);
            DrawTextFont("BUSCAR NIVEL",
                static_cast<int>(search.dialog.x + 24.0f),
                static_cast<int>(search.dialog.y + 18.0f), 28, GOLD);
            DrawTextFont("Filtra los niveles de la comunidad por nombre",
                static_cast<int>(search.dialog.x + 24.0f),
                static_cast<int>(search.dialog.y + 57.0f), 15, mutedText);
            DrawRectangleRec(search.close,
                UiButton::IsHovered(search.close)
                    ? Color{52, 57, 57, 255} : BLACK);
            DrawRectangleLinesEx(search.close, 2.0f, accentGreen);
            DrawTextFont("X",
                static_cast<int>(search.close.x +
                    (search.close.width - MeasureTextFont("X", 20)) / 2.0f),
                static_cast<int>(search.close.y + 8.0f), 20, WHITE);
            DrawRectangleRec(search.input, Color{24, 31, 40, 255});
            DrawRectangleLinesEx(search.input, 2.0f, accentGreen);
            DrawRectangle(static_cast<int>(search.input.x + 2.0f),
                static_cast<int>(search.input.y + search.input.height - 5.0f),
                static_cast<int>(search.input.width - 4.0f), 3, accentPink);
            const bool hasQuery = !m_communityQuery.empty();
            const char* shownQuery = hasQuery
                ? m_communityQuery.c_str() : "Escribe el nombre...";
            int querySize = 22;
            while (querySize > 14 &&
                MeasureTextFont(shownQuery, querySize) > search.input.width - 30.0f) {
                --querySize;
            }
            DrawTextFont(shownQuery,
                static_cast<int>(search.input.x + (hasQuery ? 14.0f : 22.0f)),
                static_cast<int>(search.input.y +
                    (search.input.height - querySize) / 2.0f),
                querySize, hasQuery ? WHITE : mutedText);
            if (std::fmod(GetTime(), 1.0) < 0.55) {
                DrawRectangle(static_cast<int>(search.input.x + 14.0f +
                    (hasQuery ? MeasureTextFont(m_communityQuery.c_str(), querySize)
                        + 2.0f : 0.0f)),
                    static_cast<int>(search.input.y + 15.0f),
                    2, 28, accentPink);
            }
            if (catalog.loading && hasQuery) {
                DrawCenteredCommunityText("Buscando...",
                    m_context->windowWidth,
                    static_cast<int>(search.dialog.y + 187.0f), 16, GOLD);
            } else {
                DrawCenteredCommunityText(
                    "Los resultados cambian mientras escribes · Enter o Esc para cerrar",
                    m_context->windowWidth,
                    static_cast<int>(search.dialog.y + 187.0f), 13, mutedText);
            }
        }
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
        const Rectangle createButton = CreateLevelButton(m_context->windowWidth);
        const bool createHovered = UiButton::IsHovered(createButton);
        DrawRectangleRec(createButton,
            createHovered ? Color{52, 57, 57, 255} : BLACK);
        DrawRectangleLinesEx(createButton, 3.0f,
            Color{129, 195, 71, 255});
        UiButton::DrawTextureFit(ui.menuCreatePlus,
            Rectangle{createButton.x + 14.0f, createButton.y + 14.0f,
                44.0f, 44.0f});

        const float guideWidth = std::min(820.0f,
            static_cast<float>(m_context->windowWidth - 40));
        const Rectangle guide{
            (m_context->windowWidth - guideWidth) / 2.0f,
            174.0f, guideWidth, 38.0f
        };
        DrawRectangleRec(guide, Fade(BLACK, 0.86f));
        DrawRectangleLinesEx(guide, 2.0f,
            Color{129, 195, 71, 255});
        const char* publishGuide =
            "Para publicar, supera tu nivel al menos una vez. Si lo editas, vuelve a superarlo.";
        DrawTextFont(publishGuide,
            static_cast<int>(guide.x +
                (guide.width - MeasureTextFont(publishGuide, 15)) / 2.0f),
            static_cast<int>(guide.y + 10.0f), 15, WHITE);

        const float listTop = CustomListTop;
        const float listBottom = static_cast<float>(m_context->windowHeight) - 76.0f;

        const float rowHeight = CustomRowHeight;
        const float rowWidth = std::min(820.0f, m_context->windowWidth - 40.0f);
        const float rowX = (m_context->windowWidth - rowWidth) / 2.0f;
        const float yPadding = CustomRowGap;
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
                m_context->windowHeight - 51,
                13,
                publication.status == PublicationStatus::Error ||
                    publication.status == PublicationStatus::Unavailable
                    ? MAROON
                    : DARKGREEN
            );
        }
        if (m_context->customLevels->GetLevels().empty()) {
            DrawCommunityNotice("Todavía no hay niveles personalizados",
                m_context->windowWidth, 249, 20, WHITE);
        } else if (!drewLevel) {
            const char* scrollText = "Usa la rueda del mouse para ver más niveles";
            DrawTextFont(
                scrollText,
                static_cast<int>((m_context->windowWidth - MeasureTextFont(scrollText, 16)) / 2),
                249,
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
            DrawRectangle(0, 0, m_context->windowWidth,
                m_context->windowHeight, Fade(BLACK, 0.62f));
            const CreateLevelDialogLayout layout = GetCreateLevelDialogLayout(
                m_context->windowWidth, m_context->windowHeight
            );
            const Rectangle& dialog = layout.dialog;
            const Color accentGreen{129, 195, 71, 255};
            const Color accentPink{237, 18, 121, 255};
            const Color mutedText{190, 201, 203, 255};
            DrawRectangleRec(Rectangle{dialog.x + 7.0f, dialog.y + 8.0f,
                dialog.width, dialog.height}, Fade(BLACK, 0.55f));
            DrawRectangleRec(dialog, Color{12, 17, 24, 250});
            DrawRectangleLinesEx(dialog, 3.0f, accentGreen);
            DrawRectangle(static_cast<int>(dialog.x + 3.0f),
                static_cast<int>(dialog.y + 84.0f),
                static_cast<int>(dialog.width - 6.0f), 3, accentPink);
            DrawTextFont("NOMBRE DEL NIVEL",
                static_cast<int>(dialog.x + 24.0f),
                static_cast<int>(dialog.y + 18.0f), 28, GOLD);
            DrawTextFont("Así aparecerá en Mis niveles",
                static_cast<int>(dialog.x + 24.0f),
                static_cast<int>(dialog.y + 57.0f), 16, mutedText);

            const Rectangle input{
                dialog.x + 24.0f, dialog.y + 105.0f,
                dialog.width - 48.0f, 58.0f
            };
            DrawRectangleRec(input, Color{24, 31, 40, 255});
            DrawRectangleLinesEx(input, 2.0f, accentGreen);
            DrawRectangle(static_cast<int>(input.x + 2.0f),
                static_cast<int>(input.y + input.height - 5.0f),
                static_cast<int>(input.width - 4.0f), 3, accentPink);
            const bool hasName = !m_newLevelName.empty();
            const char* shownName = hasName
                ? m_newLevelName.c_str() : "Escribe un nombre...";
            int nameFontSize = 22;
            while (nameFontSize > 15 &&
                MeasureTextFont(shownName, nameFontSize) > input.width - 30.0f) {
                --nameFontSize;
            }
            DrawTextFont(shownName,
                static_cast<int>(input.x + (hasName ? 14.0f : 22.0f)),
                static_cast<int>(input.y + (input.height - nameFontSize) / 2.0f),
                nameFontSize,
                hasName ? WHITE : mutedText);
            if (std::fmod(GetTime(), 1.0) < 0.55) {
                const float caretX = input.x + 14.0f +
                    (hasName ? MeasureTextFont(m_newLevelName.c_str(),
                        nameFontSize) + 2.0f : 0.0f);
                DrawRectangle(static_cast<int>(caretX),
                    static_cast<int>(input.y + 15.0f),
                    2, 28, accentPink);
            }
            const char* countText = TextFormat("%d/%d",
                static_cast<int>(CountCharacters(m_newLevelName)),
                MaxLevelNameCharacters);
            DrawTextFont(
                countText,
                static_cast<int>(input.x + input.width -
                    MeasureTextFont(countText, 15)),
                static_cast<int>(input.y + input.height + 8.0f),
                15, mutedText
            );
            DrawDialogAction(layout.cancel, ui.menuDialogCancel,
                "CANCELAR", true);
            DrawDialogAction(layout.create, ui.menuDialogCreate,
                "CREAR", hasName);
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
        {}
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
