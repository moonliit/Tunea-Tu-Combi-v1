#ifndef AUDIO_CONTROLLER_HPP
#define AUDIO_CONTROLLER_HPP

#include "app/LevelCatalog.hpp"
#include <array>
#include <raylib.h>
#include <string>
#include <unordered_map>
#include <optional>

struct Scenario {
    enum Type {
        MainMenu,
        LevelSelect,
        PlayLevel,
        VehicleBuild,
        LevelEditorBuild,
        LevelEditorMusicPreview,
    } type;
    int levelIndex = 0;
    std::string levelEditorTrackName;

    bool IsClean() const {
        return type == MainMenu || type == PlayLevel || type == LevelEditorMusicPreview;
    }
    bool IsCustom() const {
        return type == LevelSelect || type == VehicleBuild || type == LevelEditorBuild;
    }
    bool AreOnSameScene(const Scenario& other) const {
        if (type == MainMenu || type == LevelSelect) {
            return other.type == MainMenu || other.type == LevelSelect;
        }
        if (type == PlayLevel || type == VehicleBuild) {
            return other.type == PlayLevel || other.type == VehicleBuild;
        }
        if (type == LevelEditorBuild || type == LevelEditorMusicPreview) {
            return other.type == LevelEditorBuild || other.type == LevelEditorMusicPreview;
        }
        return false;
    }
    bool operator==(const Scenario& other) const {
        if (other.type == type) {
            // if in a scenario where level index matters
            if (other.type == Scenario::PlayLevel || other.type == Scenario::VehicleBuild) {
                return other.levelIndex == levelIndex;
            }
            // if in a scenario where custom music path matters
            if (other.type == Scenario::LevelEditorMusicPreview || other.type == Scenario::LevelEditorBuild) {
                return other.levelEditorTrackName == levelEditorTrackName;
            }
            return true;
        }
        return false;
    }
    bool operator!=(const Scenario& other) const {
        return !(*this == other);
    }
};

struct MusicWithPath {
    Music music;
    std::string path;

    static MusicWithPath Load(std::string path) {
        return MusicWithPath {
            .music = LoadMusicStream(path.c_str()),
            .path = std::move(path),
        };
    }
    std::optional<std::string> TryGetDualPath() const;
    MusicWithPath GetAndLoadDual() const {
        const auto dualPath = TryGetDualPath();
        if (!dualPath.has_value()) return *this;
        return Load(dualPath.value());
    }
    static std::string GetTrackName(const std::string& path);
    std::string GetTrackName() const;
};

class AudioController {
    MusicWithPath m_menuMusicClean{};
    MusicWithPath m_menuMusicCustom{};
    std::array<MusicWithPath, OfficialLevelCount> m_cleanLevelMusic{};
    std::array<MusicWithPath, OfficialLevelCount> m_customLevelMusic{};
    Sound m_radioStatic{};
    Sound m_uiClick{};
    Sound m_partSelect{};
    Sound m_partPlace{};
    Sound m_sailSound{};
    std::array<Sound, 9> m_damageSounds{};
    std::array<Sound, 4> m_breakageSounds{};
    Sound m_explosionSound{};
    std::array<Sound, 3> m_horns{};
    int m_nextHorn = 0;
    Music m_fanSound{};
    Music m_wheelSound{};
    MusicWithPath m_levelEditorMusicClean{};
    MusicWithPath m_levelEditorMusicCustom{};
    bool m_levelEditorMusicLoaded = false;
    bool m_customMusicActive = false;
    std::string m_customMusicPath;
    std::unordered_map<std::string, float> m_musicGains;
    float m_variantFade = 1.0f;
    bool m_fanActive = false;
    bool m_wheelActive = false;
    Scenario m_current{};
    Scenario m_target{};
    float m_volume = 1.0f;
    bool m_transitioning = false;
    bool m_returningToMenu = false;
    bool m_initialized = false;
    float m_damageSoundsTimeCooldown = 1.0f;
    float m_damageSoundsTimer = 0.0f;

    void BeginTransition(Scenario target);
    const MusicWithPath& GetMusicFromScenario(Scenario scenario) const;
    const MusicWithPath& SelectedMusic() const;
    float SelectedMusicGain() const;
    void ApplyMusicVolume();
    void UpdateVariantFade(float deltaTime);

public:
    void Initialize();
    void Shutdown();
    void StartLevel(int levelNumber);
    void StartLevelEditorMusic(const std::string& path, bool preview);
    void SetCurrentScenario(Scenario scenario);
    Scenario GetCurrentScenario() const;
    void ReturnToSelector();
    void ResetToMenu();
    void PlayUiClick();
    void PlayPartSelect();
    void PlayPartPlace();
    void PlaySail();
    void PlayDriverHorn();
    void PlayDamageEffect();
    void PlayBreakageEffect();
    void PlayTntExplosion(int amount);
    void SetFanActive(bool active);
    void SetWheelActive(bool active);
    void Update(float deltaTime);
};

#endif
