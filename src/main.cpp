#include "game/Game.hpp"

int main() {
    const int windowWidth = 800;
    const int windowHeight = 600;
    const std::string windowTitle = "Bad Piggies Clone - Phase 2";

    const LevelSpecs specs = {
        .initialCameraPos = {400.f, 300.f},
        .terrainControlPoints = {
            {50.0f, 450.0f},
            {250.0f, 480.0f},
            {450.0f, 400.0f},
            {650.0f, 520.0f},
            {750.0f, 520.0f}
        },
        .catmullRomResolution = 15,
        .gridDims = {10, 10},
        .gridWorldPos = {440.f, 395.f},
        .cellSize = 40.f
    };

    Game game(windowWidth, windowHeight, windowTitle, specs);
    game.Run();
    return 0;
}
