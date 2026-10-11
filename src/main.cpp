#include "physics/Box2DPhysics.hpp"
#include "physics/IPhysics.hpp"
#include <raylib-cpp/raylib-cpp.hpp>

int main() {
    // 1. Initialize Raylib window
    raylib::Window window(800, 600, "Raylib + Adapter Physics");
    SetTargetFPS(60);

    // 2. Initialize Physics via Interface
    double gravity = 9.8f * 40;
    std::unique_ptr<IPhysicsWorld> physics = std::make_unique<Box2DWorld>(Vector2D{0.0f, gravity});

    // 3. Keep references to both bodies
    Vector2D floorPos = {400.0f, 550.0f};
    Vector2D floorSize = {600.0f, 40.0f};
    auto floor = physics->CreateBox(floorPos, floorSize, 1.0f, BodyType::Static);

    Vector2D boxSize = {50.0f, 50.0f};
    auto box   = physics->CreateBox(Vector2D{400.0f, 100.0f}, boxSize, 1.0f, BodyType::Dynamic);

    // 4. Game Loop
    while (!window.ShouldClose()) {
        // Step physics simulation
        physics->Step(GetFrameTime());

        // Get live positions from the physics engine
        Vector2D currentBoxPos   = box->GetPosition();
        Vector2D currentFloorPos = floor->GetPosition();
        Vector2D currentFloorSize = floor->GetSize();

        // Render with Raylib
        BeginDrawing();
        ClearBackground(RAYWHITE);

        // Draw falling box (centered offset)
        DrawRectangle(
            (int)(currentBoxPos.x - boxSize.x / 2.0f),
            (int)(currentBoxPos.y - boxSize.y / 2.0f),
            (int)boxSize.x, (int)boxSize.y, BLUE
        );

        // Draw floor (centered offset, perfectly aligned with physics engine)
        DrawRectangle(
            (int)(currentFloorPos.x - currentFloorSize.x / 2.0f),
            (int)(currentFloorPos.y - currentFloorSize.y / 2.0f),
            (int)currentFloorSize.x, (int)currentFloorSize.y, DARKGRAY
        );

        EndDrawing();
    }

    return 0;
}
