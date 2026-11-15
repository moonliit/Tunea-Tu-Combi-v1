#include "rendering/TextureLibrary.hpp"
#include "raylib.h"

#include <array>
#include <stdexcept>
#include <string>
#include <print>
#include <format>

namespace {

struct PartTextureFile {
    PartType type;
    const char* path;
};

constexpr std::array PartTextureFiles{
    PartTextureFile{PartType::SquareBlock, "assets/textures/parts/square_block.png"},
    PartTextureFile{PartType::CircleWheel, "assets/textures/parts/circle_wheel.png"},
    PartTextureFile{PartType::TNTBarrel, "assets/textures/parts/tnt_barrel.png"},
    PartTextureFile{PartType::Fan, "assets/textures/parts/fan.png"},
    PartTextureFile{PartType::WindSail, "assets/textures/parts/wind_sail.png"},
    PartTextureFile{PartType::Driver, "assets/textures/parts/driver.png"},
    PartTextureFile{PartType::DriverBox, "assets/textures/parts/driver_block.png"}
};
constexpr const char* WheelMountPath = "assets/textures/parts/wheel_mount.png";
constexpr const char* TerrainTexturePath = "assets/textures/terrain/dirt.png";
constexpr const char* LevelGoalTexturePath = "assets/textures/level/goal.png";
constexpr const char* fontPath = "assets/fonts/Matcha Style.ttf";

Texture2D LoadSmoothTexture(const char* path, bool useBilinearFilter = true) {
    const Texture2D texture = LoadTexture(path);
    TextureFilter filter = useBilinearFilter ? TEXTURE_FILTER_BILINEAR : TEXTURE_FILTER_POINT;
    if (texture.id == 0) {
        const Texture2D texture_ = LoadTexture("assets/textures/missing.png");
        if (texture_.id != 0) {
            std::println("Warning: Could not load texture '{}', using missing texture instead.", path);
            SetTextureFilter(texture_, filter);
            return texture_;
        }
        throw std::runtime_error(
            std::string("Could not load texture: ") + path
        );
    }
    SetTextureFilter(texture, filter);
    return texture;
}

std::string GetLevelTitlePath(int levelNumber) {
    std::string num = (levelNumber < 10) ? std::format("0{}", levelNumber) : std::to_string(levelNumber);
    return std::format("assets/textures/level/title/level_{}", num);
}

UiButtonSkin LoadButtonSkin(const char* basePath) {
    return UiButtonSkin{
        .normal = LoadSmoothTexture((std::string(basePath) + "_normal.png").c_str()),
        .hover = LoadSmoothTexture((std::string(basePath) + "_hover.png").c_str()),
        .pressed = LoadSmoothTexture((std::string(basePath) + "_pressed.png").c_str()),
        .disabled = LoadSmoothTexture((std::string(basePath) + "_disabled.png").c_str()),
    };
}

void UnloadUiSkin(UiButtonSkin& skin) {
    UnloadTexture(skin.normal);
    UnloadTexture(skin.hover);
    UnloadTexture(skin.pressed);
    UnloadTexture(skin.disabled);
}

}

void DrawTextFont(const char *text, int posX, int posY, float fontSize, Color color) {
    static const Font font = LoadFont(fontPath);
    float spacing = fontSize / 10.0f;
    DrawTextEx(font, text, (Vector2){ (float)posX, (float)posY }, fontSize, spacing, color);
}

void TextureLibrary::Initialize() {
    for (const PartTextureFile& file : PartTextureFiles) {
        m_partTextures[file.type] = LoadSmoothTexture(file.path, false);
    }
    m_wheelMount = LoadSmoothTexture(WheelMountPath, false);
    for (std::size_t index = 0; index < m_backgrounds.size(); ++index) {
        m_backgrounds[index] = LoadSmoothTexture(
            GetBackgroundPath(static_cast<BackgroundId>(index))
        );
    }
    m_terrainTexture = LoadSmoothTexture(TerrainTexturePath);
    SetTextureWrap(m_terrainTexture, TEXTURE_WRAP_REPEAT);
    m_levelGoalTexture = LoadSmoothTexture(LevelGoalTexturePath);

    // UI textures (static, startup-loaded; missing files fall back to missing texture)
    m_ui.logo = LoadSmoothTexture("assets/textures/ui/branding/logo.png");

    m_ui.primary = LoadButtonSkin("assets/textures/ui/common/buttons/primary");
    m_ui.secondary = LoadButtonSkin("assets/textures/ui/common/buttons/secondary");
    m_ui.danger = LoadButtonSkin("assets/textures/ui/common/buttons/danger");
    m_ui.nav = LoadButtonSkin("assets/textures/ui/common/buttons/nav");
    m_ui.iconSquare = LoadButtonSkin("assets/textures/ui/common/buttons/icon_square");

    m_ui.arrowLeft = LoadSmoothTexture("assets/textures/ui/common/icons/arrow_left_normal.png");
    m_ui.arrowRight = LoadSmoothTexture("assets/textures/ui/common/icons/arrow_right_normal.png");
    m_ui.keyEnterBadge = LoadSmoothTexture("assets/textures/ui/common/badges/key_enter.png");

    m_ui.menuReset = LoadButtonSkin("assets/textures/ui/main_menu/buttons/reiniciar");
    m_ui.menuEditorLevels = LoadButtonSkin("assets/textures/ui/main_menu/buttons/editor_de_niveles");
    m_ui.menuCreateLevel = LoadButtonSkin("assets/textures/ui/main_menu/buttons/crear_nivel");
    m_ui.menuBack = LoadButtonSkin("assets/textures/ui/main_menu/buttons/menu");
    m_ui.menuEditAndPlay = LoadButtonSkin("assets/textures/ui/main_menu/custom_levels/editar_y_jugar");
    m_ui.menuPublish = LoadButtonSkin("assets/textures/ui/main_menu/custom_levels/publicar");
    m_ui.menuDelete = LoadButtonSkin("assets/textures/ui/main_menu/custom_levels/borrar");
    m_ui.menuDialogCancel = LoadButtonSkin("assets/textures/ui/main_menu/dialog/cancelar");
    m_ui.menuDialogCreate = LoadButtonSkin("assets/textures/ui/main_menu/dialog/crear");
    m_ui.menuToEditor = LoadButtonSkin("assets/textures/ui/main_menu/nav/to_editor");

    for (std::size_t index = 0; index < m_ui.menuLevelTitles.size(); ++index) {
        m_ui.menuLevelTitles[index] = LoadButtonSkin(
            GetLevelTitlePath(static_cast<int>(index + 1)).c_str()
        );
    }

    // main game UI
    m_ui.game.toolbarSlot = LoadButtonSkin("assets/textures/ui/game/toolbar/slot");
    m_ui.game.modePlay = LoadButtonSkin("assets/textures/ui/game/mode/play");
    m_ui.game.modeEdit = LoadButtonSkin("assets/textures/ui/game/mode/edit");
    m_ui.game.clearSelection = LoadButtonSkin("assets/textures/ui/game/actions/clear_selection_drop");
    m_ui.game.clearBuild = LoadButtonSkin("assets/textures/ui/game/actions/clear_build_trash");
    m_ui.game.enterView = LoadButtonSkin("assets/textures/ui/game/actions/enter_view");
    m_ui.game.moveLeft = LoadButtonSkin("assets/textures/ui/game/grid/move_left");
    m_ui.game.moveRight = LoadButtonSkin("assets/textures/ui/game/grid/move_right");
    m_ui.game.moveUp = LoadButtonSkin("assets/textures/ui/game/grid/move_up");
    m_ui.game.moveDown = LoadButtonSkin("assets/textures/ui/game/grid/move_down");
    m_ui.game.navMenu = LoadButtonSkin("assets/textures/ui/game/nav/menu");
    m_ui.game.navDiseno = LoadButtonSkin("assets/textures/ui/game/nav/diseno");
    m_ui.game.toolbarSelected = LoadSmoothTexture("assets/textures/ui/game/toolbar/selected.png");

    m_initialized = true;
}

void TextureLibrary::Shutdown() {
    if (!m_initialized) return;
    for (const auto& [type, texture] : m_partTextures) {
        UnloadTexture(texture);
    }
    m_partTextures.clear();
    UnloadTexture(m_wheelMount);
    for (const Texture2D& background : m_backgrounds) {
        UnloadTexture(background);
    }
    UnloadTexture(m_terrainTexture);
    UnloadTexture(m_levelGoalTexture);

    UnloadTexture(m_ui.logo);
    UnloadTexture(m_ui.arrowLeft);
    UnloadTexture(m_ui.arrowRight);
    UnloadTexture(m_ui.keyEnterBadge);
    UnloadUiSkin(m_ui.primary);
    UnloadUiSkin(m_ui.secondary);
    UnloadUiSkin(m_ui.danger);
    UnloadUiSkin(m_ui.nav);
    UnloadUiSkin(m_ui.iconSquare);
    UnloadUiSkin(m_ui.menuReset);
    UnloadUiSkin(m_ui.menuEditorLevels);
    UnloadUiSkin(m_ui.menuCreateLevel);
    UnloadUiSkin(m_ui.menuBack);
    UnloadUiSkin(m_ui.menuEditAndPlay);
    UnloadUiSkin(m_ui.menuPublish);
    UnloadUiSkin(m_ui.menuDelete);
    UnloadUiSkin(m_ui.menuDialogCancel);
    UnloadUiSkin(m_ui.menuDialogCreate);

    UnloadUiSkin(m_ui.game.toolbarSlot);
    UnloadUiSkin(m_ui.game.modePlay);
    UnloadUiSkin(m_ui.game.modeEdit);
    UnloadUiSkin(m_ui.game.clearSelection);
    UnloadUiSkin(m_ui.game.clearBuild);
    UnloadUiSkin(m_ui.game.enterView);
    UnloadUiSkin(m_ui.game.moveLeft);
    UnloadUiSkin(m_ui.game.moveRight);
    UnloadUiSkin(m_ui.game.moveUp);
    UnloadUiSkin(m_ui.game.moveDown);
    UnloadUiSkin(m_ui.game.navMenu);
    UnloadUiSkin(m_ui.game.navDiseno);
    UnloadTexture(m_ui.game.toolbarSelected);

    m_initialized = false;
}

const Texture2D& TextureLibrary::GetPartTexture(PartType type) const {
    return m_partTextures.at(type);
}
