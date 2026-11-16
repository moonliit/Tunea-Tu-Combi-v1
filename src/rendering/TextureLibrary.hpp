#ifndef RENDERING_TEXTURE_LIBRARY_HPP
#define RENDERING_TEXTURE_LIBRARY_HPP

#include "app/LevelCatalog.hpp"
#include "app/game/Part.hpp"
#include "rendering/Background.hpp"

#include <array>
#include <map>
#include <string>
#include <vector>
#include <raylib.h>

struct UiButtonSkin {
    Texture2D normal{};
    Texture2D hover{};
    Texture2D pressed{};
    Texture2D disabled{};
};

struct UiGameTextures {
    UiButtonSkin toolbarSlot;
    Texture2D editHammer{};
    std::array<Texture2D, 3> countdownNumbers{};
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
    Texture2D menuCreatePlus{};
    UiButtonSkin menuBack;
    UiButtonSkin resultsBack;
    Texture2D scoreTotalLabel{};
    std::array<Texture2D, 10> scoreDigits{};
    Texture2D scoreMinus{};
    UiButtonSkin menuEditAndPlay;
    UiButtonSkin menuPublish;
    UiButtonSkin menuDelete;
    UiButtonSkin menuDialogCancel;
    UiButtonSkin menuDialogCreate;
    UiButtonSkin menuCommunity;
    UiButtonSkin menuMyLevels;
    Texture2D communityPreviousLabel{};
    Texture2D communityNextLabel{};
    std::array<UiButtonSkin, MaxOfficialLevelCount> menuLevelTitles{};

    UiGameTextures game;
};

// cara elegible para un pasajero; name es el nombre del archivo sin extensión
struct PassengerFace {
    std::string name;
    Texture2D texture{};
};

struct ExplosionAnimation {
    std::vector<Texture2D> frames;
    float frameDuration = 0.06f;

    bool IsValid() const { return !frames.empty(); }
};

// loaded once at startup, MainGameScene is rebuilt for every level
class TextureLibrary {
private:
    std::map<PartType, Texture2D> m_partTextures;
    std::vector<PassengerFace> m_passengerFaces;
    Texture2D m_passengerSkirtless{};
    Texture2D m_passengerSkirt{};
    Texture2D m_wheelMount{};
    std::array<Texture2D, BackgroundCount> m_backgrounds{};
    Texture2D m_terrainTexture{};
    Texture2D m_levelGoalTexture{};
    UiTextures m_ui{};
    ExplosionAnimation m_explosionAnim{};
    Texture2D m_damageStarTexture{};
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
    const Texture2D& GetPassenngerSkirtless() const { return m_passengerSkirtless; }
    const Texture2D& GetPassenngerSkirt() const { return m_passengerSkirt; }
    const UiTextures& GetUi() const { return m_ui; }
    const ExplosionAnimation& GetExplosionAnim() const { return m_explosionAnim; }
    const Texture2D& GetDamageStarTexture() const { return m_damageStarTexture; }

    // todas las imágenes de assets/textures/passengers/faces, ordenadas por nombre
    const std::vector<PassengerFace>& GetPassengerFaces() const {
        return m_passengerFaces;
    }
    // nullptr si el nombre está vacío o la imagen no existe
    const Texture2D* FindPassengerFace(const std::string& name) const;
};
void DrawTextFont(const char *text, int posX, int posY, float fontSize, Color color);
float MeasureTextFont(const char *text, float fontSize);

#endif
