#include <raylib-cpp/raylib-cpp.hpp>
#include <box2d/box2d.h>
#include "Color.hpp"
#include "physics/Box2DPhysics.hpp"

// Helper to convert adapter Vector2D to raylib-cpp Vector2
raylib::Vector2 ToRaylib(Vector2D v) {
    return raylib::Vector2(static_cast<float>(v.x), static_cast<float>(v.y));
}

int main() {
    // 1. Initialize Raylib Window using raylib-cpp RAII wrapper
    const int screenWidth = 800;
    const int screenHeight = 600;
    raylib::Window window(screenWidth, screenHeight, "Box2D Adapter Test - C++23 (raylib-cpp)");
    window.SetTargetFPS(60);

    // 2. Initialize Physics World via Interface
    float gravity = 9.8f * 40;
    std::unique_ptr<IPhysicsWorld> physics = std::make_unique<Box2DWorld>(Vector2D{0.0f, gravity});

    // 3. Create Scene Elements
    // Smooth terrain based on control points
    std::vector<Vector2D> terrainPoints = {
        {0.0f, 250.0f},
        {50.0f, 450.0f},
        {250.0f, 480.0f},
        {450.0f, 400.0f},
        {650.0f, 520.0f},
        {750.0f, 520.0f},
        {850.0f, 450.0f},
        {950.0f, 480.0f},
        {1050.0f, 400.0f},
        {1250.0f, 520.0f},
        {1500.0f, 520.0f},
    };
    std::vector<Vector2D> terrainVertices = physics->CreateTerrainChain(terrainPoints, 15); // 15 subdivisiones por tramo para máxima suavidad

    // Falling Box (Dynamic Box)
    Vector2D boxSize = {150.0f, 50.0f};
    float angle = 0 * DEG2RAD;
    auto box = physics->CreateBox(Vector2D{350.0f, 150.0f}, angle, boxSize, 1.0f, BodyType::Dynamic);

    // Rolling Wheels (Dynamic Circles)
    float wheelRadius = 20.0f;
    auto wheel = physics->CreateCircle(Vector2D{300.0f, 210.0f}, wheelRadius, 1.0f, BodyType::Dynamic);

    float wheelRadius2 = 20.0f;
    auto wheel2 = physics->CreateCircle(Vector2D{400.0f, 210.0f}, wheelRadius2, 1.0f, BodyType::Dynamic);

    // Joints linking box and wheels
    auto joint1 = physics->CreateRevoluteJoint(box, wheel, wheel->GetPosition());
    auto joint2 = physics->CreateRevoluteJoint(box, wheel2, wheel2->GetPosition());

    // 4. Main Game Loop
    while (!window.ShouldClose()) {
        float dt = raylib::Window::GetFrameTime();
        if (dt > 0.033f) dt = 0.033f;

        // Apply a one-time push impulse when SPACE is pressed
        if (raylib::Keyboard::IsKeyPressed(KEY_SPACE)) {
            auto boxConcrete = std::dynamic_pointer_cast<Box2DRigidBody>(box);
            if (boxConcrete) {
                float mag = 300;
                b2Body_ApplyLinearImpulseToCenter(boxConcrete->GetInternalId(), b2Vec2{mag, 0}, true);
            }
        }

        physics->Step(dt);

        // Fetch current synced positions from physics engine
        Vector2D boxPos   = box->GetPosition();
        Vector2D wheelPos = wheel->GetPosition();
        Vector2D wheelPos2 = wheel2->GetPosition();

        // --- Render ---
        window.BeginDrawing();
        window.ClearBackground(raylib::Color::RayWhite());

        // Draw instructions
        raylib::DrawText(
            "Testing Box2D Adapter with raylib-cpp",
            20,
            20,
            20,
            raylib::Color::DarkGray()
        );

        // Draw smooth terrain on screen
        for (size_t i = 0; i < terrainVertices.size() - 1; ++i) {
            ToRaylib(terrainVertices[i]).DrawLine(
                ToRaylib(terrainVertices[i + 1]),
                4.0f,
                raylib::Color::DarkGray()
            );
        }

        // Draw Dynamic Box
        raylib::Rectangle boxRect(static_cast<float>(boxPos.x), static_cast<float>(boxPos.y), static_cast<float>(boxSize.x), static_cast<float>(boxSize.y));
        boxRect.Draw(
            raylib::Vector2(static_cast<float>(boxSize.x / 2), static_cast<float>(boxSize.y / 2)),
            box->GetRotation() * RAD2DEG,
            raylib::Color::Blue()
        );

        // Draw Dynamic Circles (Wheels) using raylib-cpp Vector2 methods
        ToRaylib(wheelPos).DrawCircle(
            wheelRadius,
            raylib::Color::Red()
        );
        ToRaylib(wheelPos2).DrawCircle(
            wheelRadius2,
            raylib::Color::Red()
        );

        // Draw lines representing joint connections
        ToRaylib(boxPos).DrawLine(
            ToRaylib(wheelPos),
            raylib::Color::Green()
        );

        ToRaylib(boxPos).DrawLine(
            ToRaylib(wheelPos2),
            raylib::Color::Green()
        );

        window.EndDrawing();
    }

    // Window closes automatically via raylib-cpp destructor (RAII)
    return 0;
}
