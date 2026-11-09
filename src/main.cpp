#include "raylib.h"
#include "physics/engine/World.hpp"
#include "physics/engine/RigidBodies.hpp"
#include <memory>

int main() {
    const int screenWidth = 800;
    const int screenHeight = 600;
    InitWindow(screenWidth, screenHeight, "Custom 2D Physics Engine - Single Pendulum Test");
    SetTargetFPS(60);

    Physics::World world(
        Math::Vector2D(0.0f, 400.f), // Gravity pulling down
        10,                          // subStepCount
        8,                           // velocityIterations
        3                            // positionIterations
    );

    // 1. Create a static anchor box at the top center
    auto _anchorBox = world.CreateBox(
        Math::Vector2D(400.0f, 200.0f),
        0.0f,
        Math::Vector2D(30.0f, 30.0f),
        0.0f,
        BodyType::Static
    );
    auto anchorBox = std::static_pointer_cast<Physics::BoxRigidBody>(_anchorBox);

    // 2. Create a single dynamic pendulum bob hanging below it
    auto _bobBox = world.CreateBox(
        Math::Vector2D(400.0f, 350.0f),
        0.0f,
        Math::Vector2D(50.0f, 50.0f),
        2.0f,
        BodyType::Dynamic
    );
    auto bobBox = std::static_pointer_cast<Physics::BoxRigidBody>(_bobBox);

    // 3. Connect them with a Revolute Joint at the anchor point (top of the bob / bottom of the anchor)
    Math::Vector2D jointAnchor(400.0f, 200.0f);
    world.CreateRevoluteJoint(anchorBox, bobBox, jointAnchor);

    while (!WindowShouldClose()) {
        float dt = 1.0f / 60.0f;
        world.Step(dt);

        BeginDrawing();
        ClearBackground(RAYWHITE);

        DrawText("Single Pendulum Revolute Joint Test", 20, 20, 20, DARKGRAY);

        // Draw Static Anchor
        {
            const auto& t = anchorBox->GetTransform();
            DrawRectanglePro(
                { t.position.x, t.position.y, 30.0f, 30.0f },
                Vector2{ 15.0f, 15.0f },
                float(t.rotation) * RAD2DEG,
                DARKGRAY
            );
        }

        // Draw Pendulum Bob
        {
            const auto& t = bobBox->GetTransform();
            Math::Vector2D size = bobBox->GetSize();
            Rectangle rec = { t.position.x, t.position.y, size.x, size.y };

            DrawRectanglePro(rec, Vector2{ size.x * 0.5f, size.y * 0.5f }, float(t.rotation) * RAD2DEG, BLUE);

            // Draw joint pivot pin at the anchor coordinate
            DrawCircleV(Vector2{ 400.0f, 200.0f }, 5.0f, RED);
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
