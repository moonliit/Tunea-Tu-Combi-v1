#ifndef GAME_APP_STATE_HPP
#define GAME_APP_STATE_HPP

#include <stdexcept>

enum class AppState {
    Build,
    View,
    Play,
    Results
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
    bool IsViewMode() const { return m_currentState == AppState::View; }
    bool IsResultsMode() const {
        return m_currentState == AppState::Results;
    }

    void EnterPlay() {
        if (!IsBuildMode()) {
            throw std::logic_error("Cannot enter Play mode from the current state");
        }
        m_currentState = AppState::Play;
    }

    void EnterView() {
        if (!IsBuildMode()) {
            throw std::logic_error("Cannot enter View mode from the current state");
        }
        m_currentState = AppState::View;
    }

    void EnterBuild() {
        if (!IsPlayMode() && !IsViewMode() && !IsResultsMode()) {
            throw std::logic_error("Cannot enter Build mode from the current state");
        }
        m_currentState = AppState::Build;
    }

    void EnterResults() {
        if (!IsPlayMode()) {
            throw std::logic_error(
                "Cannot enter Results mode from the current state"
            );
        }
        m_currentState = AppState::Results;
    }
};

#endif
