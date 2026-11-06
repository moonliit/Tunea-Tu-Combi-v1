#ifndef EDITOR_SCENE_HPP
#define EDITOR_SCENE_HPP

#include "app/I_Scene.hpp"

#include <raylib.h>

class EditorScene final : public I_Scene {
private:
    int m_screenWidth;
    int m_screenHeight;
    Camera2D m_camera;
    // EditorStateMachine m_stateMachine;

    // void ResetToBuildMode();
    // void ResetEditorCamera();
    // void TransitionToPlayMode();

public:
    EditorScene(int width, int height);
    void ProcessInput(float deltaTime) override;
    void Update(float deltaTime) override;
    void Render() const override;
};

#endif
