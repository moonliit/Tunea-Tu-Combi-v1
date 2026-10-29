#ifndef GAME_HPP
#define GAME_HPP

#include "physics/I_Physics.hpp"
#include "game/Level.hpp"
#include "game/Part.hpp"

#include <raylib.h>
#include <optional>
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
    Camera2D m_camera;

    float m_gravity;
    GameState m_currentState;
    std::vector<Part> m_activeParts;
    std::unique_ptr<I_PhysicsWorld> m_physics;
    std::optional<PartType> m_currentPartType;
    std::vector<Math::Vector2D> m_terrainVertices;

    Level m_level;

    void SetPartType(PartType type);

    void ProcessInput(float dt);
    void Update(float dt);
    void Render();
    void ResetToEditMode();
    void TransitionToPlayMode();

public:
    Game(int width, int height, const std::string& title, LevelSpecs specs);
    ~Game() = default;

    void Run();
};

#endif
