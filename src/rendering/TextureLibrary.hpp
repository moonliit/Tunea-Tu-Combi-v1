#ifndef RENDERING_TEXTURE_LIBRARY_HPP
#define RENDERING_TEXTURE_LIBRARY_HPP

#include "app/game/Part.hpp"
#include "rendering/Background.hpp"

#include <array>
#include <map>
#include <raylib.h>

// loaded once at startup, MainGameScene is rebuilt for every level
class TextureLibrary {
private:
    std::map<PartType, Texture2D> m_partTextures;
    Texture2D m_wheelMount{};
    std::array<Texture2D, BackgroundCount> m_backgrounds{};
    bool m_initialized = false;

public:
    // needs the window open, throws if a file is missing
    void Initialize();
    void Shutdown();

    const Texture2D& GetPartTexture(PartType type) const;
    // triangle that attaches the wheel to its cell
    const Texture2D& GetWheelMount() const { return m_wheelMount; }
    const Texture2D& GetBackground(BackgroundId id) const {
        return m_backgrounds[static_cast<std::size_t>(id)];
    }
};

#endif
