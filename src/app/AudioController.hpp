#ifndef AUDIO_CONTROLLER_HPP
#define AUDIO_CONTROLLER_HPP

#include <array>
#include <raylib.h>

class AudioController {
    std::array<Music, 4> m_music{};
    Sound m_radioStatic{};
    Sound m_uiClick{};
    Sound m_partSelect{};
    Sound m_partPlace{};
    Sound m_sailSound{};
    Music m_fanSound{};
    Music m_wheelSound{};
    bool m_fanActive = false;
    bool m_wheelActive = false;
    int m_current = 0;
    int m_target = 0;
    float m_volume = 1.0f;
    bool m_transitioning = false;
    bool m_returningToMenu = false;
    bool m_initialized = false;

    void BeginTransition(int target);

public:
    void Initialize();
    void Shutdown();
    void StartLevel(int levelNumber);
    void ReturnToSelector();
    void ResetToMenu();
    void PlayUiClick();
    void PlayPartSelect();
    void PlayPartPlace();
    void PlaySail();
    void SetFanActive(bool active);
    void SetWheelActive(bool active);
    void Update(float deltaTime);
};

#endif
