#ifndef EDITOR_STATE_HPP
#define EDITOR_STATE_HPP

#include <stdexcept>

enum class EditorState {
    Build,
    View,
    Play,
    Results
};

class EditorStateMachine {
private:
    EditorState m_currentState;

public:
    explicit EditorStateMachine(EditorState initialState = EditorState::Build):
        m_currentState(initialState) {}

    EditorState GetState() const { return m_currentState; }
    bool IsBuildMode() const { return m_currentState == EditorState::Build; }
    bool IsPlayMode() const { return m_currentState == EditorState::Play; }
    bool IsViewMode() const { return m_currentState == EditorState::View; }
    bool IsResultsMode() const {
        return m_currentState == EditorState::Results;
    }

    void EnterPlay() {
        if (!IsBuildMode()) {
            throw std::logic_error("Cannot enter Play mode from the current state");
        }
        m_currentState = EditorState::Play;
    }

    void EnterView() {
        if (!IsBuildMode()) {
            throw std::logic_error("Cannot enter View mode from the current state");
        }
        m_currentState = EditorState::View;
    }

    void EnterBuild() {
        if (!IsPlayMode() && !IsViewMode() && !IsResultsMode()) {
            throw std::logic_error("Cannot enter Build mode from the current state");
        }
        m_currentState = EditorState::Build;
    }

    void EnterResults() {
        if (!IsPlayMode()) {
            throw std::logic_error("Cannot enter Results mode from the current state");
        }
        m_currentState = EditorState::Results;
    }
};

#endif
