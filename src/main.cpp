#include "raylib.h"
#include "rlgl.h"
#include "physics/engine/World.hpp"
#include "physics/engine/RigidBodies.hpp"
#include <memory>
#include <vector>
#include <array>
using namespace Math;

static Vector2 ToRaylib(const Vector2D& v) {
    return { v.x, v.y };
}
static void DrawTriangleBody(
    const std::shared_ptr<Physics::TriangleRigidBody>& body,
    Color color
) {
    const Physics::Transform& t = body->GetTransform();
    const Vector2D size = body->GetSize();

    const float halfWidth = size.x * 0.5f;
    const float halfHeight = size.y * 0.5f;

    const Vector2 tip{-halfWidth, 0.0f};
    const Vector2 baseTop{halfWidth, -halfHeight};
    const Vector2 baseBottom{halfWidth, halfHeight};

    const float rotationDeg = float(t.rotation) * RAD2DEG;

    rlPushMatrix();

    rlTranslatef(t.position.x, t.position.y, 0.0f);
    rlRotatef(rotationDeg, 0.0f, 0.0f, 1.0f);

    DrawTriangle(
        tip,
        baseBottom,
        baseTop,
        color
    );

    DrawTriangleLines(
        tip,
        baseBottom,
        baseTop,
        BLACK
    );

    rlPopMatrix();

    // Optional: show the body's orientation.
    DrawLineEx(
        { t.position.x, t.position.y },
        {
            t.position.x + cosf(t.rotation) * halfHeight,
            t.position.y + sinf(t.rotation) * halfHeight
        },
        2.0f,
        RED
    );

    DrawCircleV(
        { t.position.x, t.position.y },
        3.0f,
        RED
    );
}

int main() {
    const int screenWidth = 1000;
    const int screenHeight = 700;

    InitWindow(
        screenWidth,
        screenHeight,
        "Custom Physics Engine - Triangle Collision Test"
    );

    SetTargetFPS(60);

    Physics::World world(
        Vector2D(0.0f, 500.0f), // Gravity
        10,                     // Substeps
        8,                      // Velocity iterations
        3                       // Position iterations
    );

    // -------------------------------------------------
    // Terrain
    // -------------------------------------------------

    const std::vector<Vector2D> terrainPoints = {
        { 50.0f, 550.0f },
        { 350.0f, 550.0f },
        { 500.0f, 450.0f },
        { 650.0f, 550.0f },
        { 950.0f, 550.0f }
    };

    world.CreateTerrainChain(terrainPoints);

    // -------------------------------------------------
    // Dynamic triangles
    // -------------------------------------------------

    std::vector<std::shared_ptr<Physics::TriangleRigidBody>> triangles;

    auto CreateTestTriangle = [&](Vector2D position,
                                  float rotation,
                                  Vector2D size,
                                  float mass) {
        auto body = world.CreateTriangle(
            position,
            rotation,
            size,
            mass,
            BodyType::Dynamic
        );

        auto triangle =
            std::static_pointer_cast<Physics::TriangleRigidBody>(body);

        triangles.push_back(triangle);
    };

    CreateTestTriangle(
        { 180.0f, 100.0f },
        0.0f,
        { 60.0f, 60.0f },
        2.0f
    );

    CreateTestTriangle(
        { 300.0f, 50.0f },
        0.3f,
        { 80.0f, 70.0f },
        2.0f
    );

    CreateTestTriangle(
        { 430.0f, 100.0f },
        -0.2f,
        { 70.0f, 70.0f },
        2.0f
    );

    CreateTestTriangle(
        { 600.0f, 50.0f },
        0.5f,
        { 90.0f, 70.0f },
        2.0f
    );

    // -------------------------------------------------
    // Camera
    // -------------------------------------------------

    Camera2D camera = { 0 };

    camera.target = { 500.0f, 350.0f };
    camera.offset = {
        screenWidth * 0.5f,
        screenHeight * 0.5f
    };
    camera.rotation = 0.0f;
    camera.zoom = 1.0f;

    // -------------------------------------------------
    // Main loop
    // -------------------------------------------------

    while (!WindowShouldClose()) {
        float dt = GetFrameTime();

        // Camera controls.
        float panSpeed = 400.0f * dt;

        if (IsKeyDown(KEY_W)) camera.target.y -= panSpeed;
        if (IsKeyDown(KEY_S)) camera.target.y += panSpeed;
        if (IsKeyDown(KEY_A)) camera.target.x -= panSpeed;
        if (IsKeyDown(KEY_D)) camera.target.x += panSpeed;

        if (IsKeyDown(KEY_Q)) camera.zoom *= 1.0f + 1.0f * dt;
        if (IsKeyDown(KEY_E)) camera.zoom *= 1.0f - 1.0f * dt;

        // Spawn another triangle.
        if (IsKeyPressed(KEY_SPACE)) {
            CreateTestTriangle(
                { 500.0f, 100.0f },
                0.0f,
                { 70.0f, 70.0f },
                2.0f
            );
        }

        world.Step(dt);

        // Rendering.
        BeginDrawing();
        ClearBackground(RAYWHITE);

        BeginMode2D(camera);

        // Grid.
        for (int x = 0; x <= 1000; x += 50) {
            DrawLine(x, 0, x, 700, Fade(LIGHTGRAY, 0.5f));
        }

        for (int y = 0; y <= 700; y += 50) {
            DrawLine(0, y, 1000, y, Fade(LIGHTGRAY, 0.5f));
        }

        // Terrain.
        for (size_t i = 0; i + 1 < terrainPoints.size(); ++i) {
            DrawLineEx(
                ToRaylib(terrainPoints[i]),
                ToRaylib(terrainPoints[i + 1]),
                6.0f,
                DARKGRAY
            );
        }

        // Triangles.
        const std::array<Color, 4> colors = {
            BLUE,
            RED,
            GREEN,
            PURPLE
        };

        for (size_t i = 0; i < triangles.size(); ++i) {
            DrawTriangleBody(
                triangles[i],
                colors[i % colors.size()]
            );
        }

        EndMode2D();

        // HUD.
        DrawText("Triangle-Triangle Collision Test", 20, 20, 20, DARKGRAY);
        DrawText("WASD: Move camera", 20, 50, 16, GRAY);
        DrawText("Q/E: Zoom", 20, 70, 16, GRAY);
        DrawText("SPACE: Spawn triangle", 20, 90, 16, GRAY);

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
