#include "physics/Box2DPhysics.hpp" // Or whatever your header is named
#include "raylib.h"
#include <memory>
#include <iostream>
#include <raylib.h>

Vector2 Vector2DToRaylib(Vector2D v) {
    return Vector2{ static_cast<float>(v.x), static_cast<float>(v.y) };
}

int main() {
    // 1. Initialize Raylib Window
    const int screenWidth = 800;
    const int screenHeight = 600;
    InitWindow(screenWidth, screenHeight, "Box2D-Lite Adapter Test - C++23");
    SetTargetFPS(60);

    // 2. Initialize Physics World via Interface (Gravity pulling down)
    float gravity = 9.8f * 35;
    std::unique_ptr<IPhysicsWorld> physics = std::make_unique<Box2DWorld>(Vector2D{0.0f, gravity});

    // 3. Create Scene Elements
    // Floor (Static Box)
    Vector2D floorPos = {400.0f, 520.0f};
    Vector2D floorSize = {700.0f, 40.0f};
    float floorAngle = 5 * DEG2RAD;
    auto floor = physics->CreateBox(floorPos, floorAngle, floorSize, 0.0f, BodyType::Static);

    // Falling Box (Dynamic Box)
    Vector2D boxSize = {150.0f, 50.0f};
    float angle = 0 * DEG2RAD;
    auto box = physics->CreateBox(Vector2D{350.0f, 150.0f}, angle, boxSize, 1.0f, BodyType::Dynamic);

    // Rolling Wheel (Dynamic Circle)
    float wheelRadius = 20.0f;
    auto wheel = physics->CreateCircle(Vector2D{300.0f, 210.0f}, wheelRadius, 1.0f, BodyType::Dynamic);

    float wheelRadius2 = 20.0f;
    auto wheel2 = physics->CreateCircle(Vector2D{400.0f, 210.0f}, wheelRadius2, 1.0f, BodyType::Dynamic);

    // Optional: Joint linking the box and the wheel together to test constraints
    // (Anchor point halfway between them)
    auto joint1 = physics->CreateRevoluteJoint(box, wheel, wheel->GetPosition());
    auto joint2 = physics->CreateRevoluteJoint(box, wheel2, wheel2->GetPosition());

    // 4. Main Game Loop
    while (!WindowShouldClose()) {
        // Step physics simulation using fixed or frame time (capped safely)
        float dt = GetFrameTime();
        if (dt > 0.033f) dt = 0.033f; // Cap max delta time
        physics->Step(dt);

        // Fetch current synced positions from physics engine
        Vector2D boxPos   = box->GetPosition();
        Vector2D wheelPos = wheel->GetPosition();
        Vector2D wheelPos2 = wheel2->GetPosition();
        Vector2D fPos     = floor->GetPosition();
        Vector2D fSize    = floor->GetSize();

        // --- Render ---
        BeginDrawing();
        ClearBackground(RAYWHITE);

        // Draw instructions
        DrawText("Testing Box2D-Lite Adapter: Boxes, Circles & Joints", 20, 20, 20, DARKGRAY);

        // Draw Floor
        DrawRectanglePro(
            Rectangle {
                static_cast<float>(fPos.x),
                static_cast<float>(fPos.y),
                static_cast<float>(fSize.x),
                static_cast<float>(fSize.y),
            },
            Vector2 {
                static_cast<float>(fSize.x/2),
                static_cast<float>(fSize.y/2),
            },
            floor->GetRotation() * RAD2DEG,
            DARKGRAY
        );

        // Draw Dynamic Box
        DrawRectanglePro(
            Rectangle {
                static_cast<float>(boxPos.x),
                static_cast<float>(boxPos.y),
                static_cast<float>(boxSize.x),
                static_cast<float>(boxSize.y),
            },
            Vector2 {
                static_cast<float>(boxSize.x/2),
                static_cast<float>(boxSize.y/2),
            },
            box->GetRotation() * RAD2DEG,
            BLUE
        );

        // Draw Dynamic Circle (Wheel)
        DrawCircle(
            static_cast<int>(wheelPos.x),
            static_cast<int>(wheelPos.y),
            wheelRadius,
            RED
        );

        DrawCircle(
            static_cast<int>(wheelPos2.x),
            static_cast<int>(wheelPos2.y),
            wheelRadius2,
            RED
        );

        // Draw line representing the joint anchor connection
        DrawLineV(
            Vector2DToRaylib(boxPos), // helper or direct conversion
            Vector2DToRaylib(wheelPos),
            GREEN
        );

        DrawLineV(
            Vector2DToRaylib(boxPos), // helper or direct conversion
            Vector2DToRaylib(wheelPos2),
            GREEN
        );

        EndDrawing();
    }

    CloseWindow();
    return 0;
}
