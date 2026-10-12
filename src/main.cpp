#include <raylib-cpp/raylib-cpp.hpp>
#include <box2d/box2d.h>
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
    // Floor (Static Box)
    Vector2D floorPos = {400.0f, 520.0f};
    Vector2D floorSize = {700.0f, 40.0f};
    float floorAngle = -5 * DEG2RAD;
    auto floor = physics->CreateBox(floorPos, floorAngle, floorSize, 0.0f, BodyType::Static);

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
                float yVal = mag * sin(floorAngle);
                float xVal = mag * cos(floorAngle);
                b2Body_ApplyLinearImpulseToCenter(boxConcrete->GetInternalId(), b2Vec2{xVal, yVal}, true);
            }
        }

        physics->Step(dt);

        // Fetch current synced positions from physics engine
        Vector2D boxPos   = box->GetPosition();
        Vector2D wheelPos = wheel->GetPosition();
        Vector2D wheelPos2 = wheel2->GetPosition();
        Vector2D fPos     = floor->GetPosition();
        Vector2D fSize    = floor->GetSize();

        // --- Render ---
        window.BeginDrawing();
        window.ClearBackground(raylib::Color::RayWhite());

        // Draw instructions
        raylib::DrawText("Testing Box2D Adapter with raylib-cpp", 20, 20, 20, DARKGRAY);

        // Draw Floor using raylib-cpp Rectangle class
        raylib::Rectangle floorRect(static_cast<float>(fPos.x), static_cast<float>(fPos.y), static_cast<float>(fSize.x), static_cast<float>(fSize.y));
        floorRect.Draw(
            raylib::Vector2(static_cast<float>(fSize.x / 2), static_cast<float>(fSize.y / 2)),
            floor->GetRotation() * RAD2DEG,
            raylib::Color::DarkGray()
        );

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
