#ifndef AUDIO_CONTROLLER_HPP
#define AUDIO_CONTROLLER_HPP

#include "app/LevelCatalog.hpp"
#include <array>
#include <raylib.h>
#include <string>

class AudioController {
    std::array<Music, OfficialLevelCount + 1> m_music{};
    Sound m_radioStatic{};
    Sound m_uiClick{};
    Sound m_partSelect{};
    Sound m_partPlace{};
    Sound m_sailSound{};
    std::array<Sound, 3> m_horns{};
    int m_nextHorn = 0;
    Music m_fanSound{};
    Music m_wheelSound{};
    Music m_customMusic{};
    Music m_cleanMusic{};
    bool m_cleanMusicLoaded = false;
    bool m_customMusicLoaded = false;
    bool m_customMusicActive = false;
    bool m_playMode = false;
    float m_variantFade = 1.0f;
    bool m_fanActive = false;
    bool m_wheelActive = false;
    int m_current = 0;
    int m_target = 0;
    float m_volume = 1.0f;
    bool m_transitioning = false;
    bool m_returningToMenu = false;
    bool m_initialized = false;

    void BeginTransition(int target);
    void LoadCleanMusic(const std::string& editedPath);
    Music& SelectedMusic();
    void UpdateVariantFade(float deltaTime);

public:
    void Initialize();
    void Shutdown();
    void StartLevel(int levelNumber);
    void StartCustomMusic(const std::string& path);
    void SetPlayMode(bool playing);
    void ReturnToSelector();
    void ResetToMenu();
    void PlayUiClick();
    void PlayPartSelect();
    void PlayPartPlace();
    void PlaySail();
    void PlayDriverHorn();
    void SetFanActive(bool active);
    void SetWheelActive(bool active);
    void Update(float deltaTime);
};

#endif
