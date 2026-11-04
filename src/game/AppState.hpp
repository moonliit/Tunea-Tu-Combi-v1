#ifndef GAME_APP_STATE_HPP
#define GAME_APP_STATE_HPP

#include <stdexcept>

enum class AppState {
    Build,
    Play
};

class AppStateMachine {
private:
    AppState m_currentState;

public:
    explicit AppStateMachine(AppState initialState = AppState::Build):
        m_currentState(initialState) {}

    AppState GetState() const { return m_currentState; }
    bool IsBuildMode() const { return m_currentState == AppState::Build; }
    bool IsPlayMode() const { return m_currentState == AppState::Play; }

    void EnterPlay() {
        if (!IsBuildMode()) {
            throw std::logic_error("Cannot enter Play mode from the current state");
        }
        m_currentState = AppState::Play;
    }

    void EnterBuild() {
        if (!IsPlayMode()) {
            throw std::logic_error("Cannot enter Build mode from the current state");
        }
        m_currentState = AppState::Build;
    }
};

#endif
