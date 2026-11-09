#include "raylib.h"
#include "physics/engine/World.hpp"
#include "physics/engine/RigidBodies.hpp"
#include <vector>
#include <memory>

int main() {
    // 1. Initialize Raylib Window (800x600 pixels)
    const int screenWidth = 800;
    const int screenHeight = 600;
    InitWindow(screenWidth, screenHeight, "Custom 2D Physics Engine - Raylib");
    SetTargetFPS(60);

    // 2. Create Physics World
    // Note: Positive Y gravity pulls objects down toward the bottom of the Raylib window
    Physics::World world(
        Math::Vector2D(0.0f, 400.f),
        10, // subStepCount
        8,  // velocityIterations
        3   // positionIterations
    );

    // 3. Create Terrain Floor & Walls
    // Screen center is roughly around Y=300, floor at Y=500
    auto terrainPoints = world.CreateTerrainChain({
        Math::Vector2D(100.0f, 500.0f),
        Math::Vector2D(400.0f, 550.0f),
        Math::Vector2D(700.0f, 500.0f)
    }, false);

    // 4. Spawn Dynamic Test Objects
    // A falling box
    auto _box = world.CreateBox(
        Math::Vector2D(350.0f, 250.0f), // position (x, y)
        0.0f,                           // rotation in radians
        Math::Vector2D(60.0f, 60.0f),   // size (width, height)
        2.0f,                           // mass
        BodyType::Dynamic
    );
    auto box = static_pointer_cast<Physics::BoxRigidBody>(_box);

    int boxCount = 5;
    std::vector<std::shared_ptr<Physics::BoxRigidBody>> boxes(boxCount, nullptr);
    for (int i = 0; i < boxCount; ++i) {
        auto _box = world.CreateBox(
            Math::Vector2D(250.0f, 250.0f - i * 100.0f), // position (x, y)
            0.0f,                           // rotation in radians
            Math::Vector2D(60.0f, 60.0f),   // size (width, height)
            2.0f,                           // mass
            BodyType::Dynamic
        );
        boxes[i] = static_pointer_cast<Physics::BoxRigidBody>(_box);
    }

    // A falling circle next to it
    auto _circle = world.CreateCircle(
        Math::Vector2D(480.0f, 200.0f),  // position
        30.0f,                          // radius
        1.0f,                           // mass
        BodyType::Dynamic
    );
    auto circle = static_pointer_cast<Physics::CircleRigidBody>(_circle);

    // 5. Main Game Loop
    while (!WindowShouldClose()) {
        // Fixed timestep for physics stability
        float dt = 1.0f / 60.0f;
        world.Step(dt);

        // --- Rendering ---
        BeginDrawing();
        ClearBackground(RAYWHITE);

        // Draw instructions
        DrawText("2D Physics Engine Simulation", 20, 20, 20, DARKGRAY);

        // Draw Terrain Chain
        for (size_t i = 0; i < terrainPoints.size() - 1; ++i) {
            Math::Vector2D p1 = terrainPoints[i];
            Math::Vector2D p2 = terrainPoints[i + 1];
            DrawLineV(p1.ToRayVec(), p2.ToRayVec(), DARKGREEN);
            DrawCircleV(p1.ToRayVec(), 4.0f, DARKGREEN);
        }

        // Draw the Box
        {
            const Physics::Transform& t = box->GetTransform();
            Math::Vector2D size = box->GetSize();
            Rectangle rec = { t.position.x, t.position.y, size.x, size.y };
            // Raylib expects rotation in degrees
            DrawRectanglePro(rec, Vector2{size.x * 0.5f, size.y * 0.5f}, float(t.rotation) * RAD2DEG, BLUE);
            for (const auto& b : boxes) {
                const Physics::Transform& t = b->GetTransform();
                Math::Vector2D size = b->GetSize();
                Rectangle rec = { t.position.x, t.position.y, size.x, size.y };
                DrawRectanglePro(rec, Vector2{size.x * 0.5f, size.y * 0.5f}, float(t.rotation) * RAD2DEG, BLUE);
            }
        }

        // Draw the Circle
        {
            const Physics::Transform& t = circle->GetTransform();
            Math::RealNum radius = circle->GetRadius();
            DrawCircleV(Vector2{t.position.x, t.position.y}, radius, RED);

            // Draw a small orientation indicator line on the circle
            Math::Vector2D dir(std::cos(t.rotation), std::sin(t.rotation));
            Math::Vector2D edge = t.position + dir * radius;
            DrawLineV(Vector2{t.position.x, t.position.y}, Vector2{edge.x, edge.y}, DARKPURPLE);
        }

        EndDrawing();
    }

    CloseWindow();
    return 0;
}

// #include "app/App.hpp"
//
// int main() {
//     const int windowWidth = 800;
//     const int windowHeight = 600;
//     const std::string windowTitle = "Bad Piggies Clone";
//
//     App app(windowWidth, windowHeight, windowTitle);
//     app.Run();
//     return 0;
// }
