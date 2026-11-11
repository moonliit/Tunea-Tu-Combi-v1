#include "rendering/TextureLibrary.hpp"

#include <array>
#include <stdexcept>
#include <string>

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
    PartTextureFile{PartType::Driver, "assets/textures/parts/driver.png"}
};
constexpr const char* WheelMountPath = "assets/textures/parts/wheel_mount.png";

Texture2D LoadSmoothTexture(const char* path) {
    const Texture2D texture = LoadTexture(path);
    if (texture.id == 0) {
        throw std::runtime_error(
            std::string("Could not load texture: ") + path
        );
    }
    // point filtering looks jagged when scaled down
    SetTextureFilter(texture, TEXTURE_FILTER_BILINEAR);
    return texture;
}

}

void TextureLibrary::Initialize() {
    for (const PartTextureFile& file : PartTextureFiles) {
        m_partTextures[file.type] = LoadSmoothTexture(file.path);
    }
    m_wheelMount = LoadSmoothTexture(WheelMountPath);
    for (std::size_t index = 0; index < m_backgrounds.size(); ++index) {
        m_backgrounds[index] = LoadSmoothTexture(
            GetBackgroundPath(static_cast<BackgroundId>(index))
        );
    }
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
    m_initialized = false;
}

const Texture2D& TextureLibrary::GetPartTexture(PartType type) const {
    return m_partTextures.at(type);
}
