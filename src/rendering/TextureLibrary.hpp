#ifndef RENDERING_TEXTURE_LIBRARY_HPP
#define RENDERING_TEXTURE_LIBRARY_HPP

#include "app/game/Part.hpp"
#include "rendering/Background.hpp"

#include <array>
#include <map>
#include <raylib.h>

struct UiButtonSkin {
    Texture2D normal{};
    Texture2D hover{};
    Texture2D pressed{};
    Texture2D disabled{};
};

struct UiGameTextures {
    UiButtonSkin toolbarSlot;
    UiButtonSkin modePlay;
    UiButtonSkin modeEdit;
    UiButtonSkin clearSelection; // DROP
    UiButtonSkin clearBuild;     // TRASH
    UiButtonSkin enterView;
    UiButtonSkin moveLeft;
    UiButtonSkin moveRight;
    UiButtonSkin moveUp;
    UiButtonSkin moveDown;
    UiButtonSkin navMenu;
    UiButtonSkin navDiseno;
    Texture2D toolbarSelected{};
};

struct UiTextures {
    Texture2D logo{};

    UiButtonSkin primary;
    UiButtonSkin secondary;
    UiButtonSkin danger;
    UiButtonSkin nav;
    UiButtonSkin iconSquare;

    Texture2D arrowLeft{};
    Texture2D arrowRight{};
    Texture2D keyEnterBadge{};

    // main menu / important screens (placeholders fine via missing-texture fallback)
    UiButtonSkin menuReset;
    UiButtonSkin menuEditorLevels;
    UiButtonSkin menuCreateLevel;
    UiButtonSkin menuBack;
    UiButtonSkin menuEditAndPlay;
    UiButtonSkin menuPublish;
    UiButtonSkin menuDelete;
    UiButtonSkin menuDialogCancel;
    UiButtonSkin menuDialogCreate;
    UiButtonSkin menuToEditor;

    UiGameTextures game;
};

// loaded once at startup, MainGameScene is rebuilt for every level
class TextureLibrary {
private:
    std::map<PartType, Texture2D> m_partTextures;
    Texture2D m_wheelMount{};
    std::array<Texture2D, BackgroundCount> m_backgrounds{};
    Texture2D m_terrainTexture{};
    Texture2D m_levelGoalTexture{};
    UiTextures m_ui{};
    Font m_font{};
    bool m_initialized = false;

public:
    // needs the window open, loads a missing-texture fallback if a file is absent
    void Initialize();
    void Shutdown();

    const Texture2D& GetPartTexture(PartType type) const;
    // triangle that attaches the wheel to its cell
    const Texture2D& GetWheelMount() const { return m_wheelMount; }
    const Texture2D& GetBackground(BackgroundId id) const {
        return m_backgrounds[static_cast<std::size_t>(id)];
    }
    const Texture2D& GetTerrainTexture() const { return m_terrainTexture; }
    const Texture2D& GetLevelGoalTexture() const { return m_levelGoalTexture; }
    const UiTextures& GetUi() const { return m_ui; }
};
void DrawTextFont(const char *text, int posX, int posY, float fontSize, Color color);

#endif
