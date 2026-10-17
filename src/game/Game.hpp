#ifndef GAME_HPP
#define GAME_HPP

#include "physics/IPhysics.hpp"
#include "game/EditorGrid.hpp"
#include "game/Part.hpp"

#include <raylib-cpp/raylib-cpp.hpp>
#include <memory>
#include <vector>

enum class GameState {
    Edit,
    Play
};

class Game {
private:
    int m_screenWidth;
    int m_screenHeight;
    raylib::Window m_window;
    raylib::Camera2D m_camera;

    GameState m_currentState;
    PartType m_currentPartType;
    EditorGrid m_grid;

    std::unique_ptr<IPhysicsWorld> m_physics;
    std::vector<Math::Vector2D> m_terrainControlPoints;
    std::vector<Math::Vector2D> m_terrainVertices;
    std::vector<Part> m_activeParts;

    float m_gravity;

    void ProcessInput(float dt);
    void Update(float dt);
    void Render();
    void ResetToEditMode();
    void TransitionToPlayMode();

public:
    Game(int width, int height, const std::string& title);
    ~Game() = default;

    void Run();
};

#endif
