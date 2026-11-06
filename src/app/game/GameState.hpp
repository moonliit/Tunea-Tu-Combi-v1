#ifndef GAME_STATE_HPP
#define GAME_STATE_HPP

#include <stdexcept>

enum class GameState {
    Build,
    View,
    Play,
    Results
};

class GameStateMachine {
private:
    GameState m_currentState;

public:
    explicit GameStateMachine(GameState initialState = GameState::Build):
        m_currentState(initialState) {}

    GameState GetState() const { return m_currentState; }
    bool IsBuildMode() const { return m_currentState == GameState::Build; }
    bool IsPlayMode() const { return m_currentState == GameState::Play; }
    bool IsViewMode() const { return m_currentState == GameState::View; }
    bool IsResultsMode() const {
        return m_currentState == GameState::Results;
    }

    void EnterPlay() {
        if (!IsBuildMode()) {
            throw std::logic_error("Cannot enter Play mode from the current state");
        }
        m_currentState = GameState::Play;
    }

    void EnterView() {
        if (!IsBuildMode()) {
            throw std::logic_error("Cannot enter View mode from the current state");
        }
        m_currentState = GameState::View;
    }

    void EnterBuild() {
        if (!IsPlayMode() && !IsViewMode() && !IsResultsMode()) {
            throw std::logic_error("Cannot enter Build mode from the current state");
        }
        m_currentState = GameState::Build;
    }

    void EnterResults() {
        if (!IsPlayMode()) {
            throw std::logic_error("Cannot enter Results mode from the current state");
        }
        m_currentState = GameState::Results;
    }
};

#endif
