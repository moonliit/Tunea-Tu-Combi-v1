// #include "raylib.h"
// #include "raymath.h"
// #include "rlgl.h"
// #include "physics/engine/World.hpp"
// #include "physics/I_Physics.hpp"
// #include "physics/engine/RigidBodies.hpp"
// #include <vector>
// #include <memory>
// #include <algorithm>
//
// using namespace std;
// using namespace Math;
// using namespace Physics;
//
// int main() {
//     const int screenWidth = 800;
//     const int screenHeight = 600;
//     InitWindow(screenWidth, screenHeight, "Physics World Test - Triangle vs Capsule");
//     SetTargetFPS(60);
//
//     // Initialize World with gravity, substeps, and solver iterations
//     World world(Vector2D(0.0f, 980.0f), 4, 8, 3);
//
//     // Create a static terrain chain at the bottom
//     vector<Vector2D> controlPoints = {
//         Vector2D(50.0f, 520.0f),
//         Vector2D(400.0f, 550.0f),
//         Vector2D(750.0f, 520.0f)
//     };
//     world.CreateTerrainChain(controlPoints, false);
//
//     // Create a dynamic triangle in the center
//     auto triangle = world.CreateTriangle(
//         Vector2D(400.0f, 300.0f),
//         0.0f,                  // rotation
//         Vector2D(70.0f, 60.0f), // size (width, height)
//         12.0f,                 // mass
//         BodyType::Dynamic
//     );
//
//     // Create a dynamic capsule above the triangle
//     auto capsule = world.CreateCapsule(
//         Vector2D(390.0f, 150.0f),
//         0.5f,                  // initial rotation (radians)
//         40.0f,                 // height
//         15.0f,                 // radius
//         10.0f,                 // mass
//         BodyType::Dynamic
//     );
//
//     while (!WindowShouldClose()) {
//         float dt = GetFrameTime();
//         if (dt > 0.1f) dt = 0.1f;
//
//         // Step the physics simulation world
//         world.Step(dt);
//
//         // Render loop
//         BeginDrawing();
//         ClearBackground(RAYWHITE);
//
//         // Draw dynamic triangle using its collision polygon vertices
//         auto triBody = dynamic_pointer_cast<TriangleRigidBody>(triangle);
//         if (triBody) {
//             auto poly = triBody->GetCollisionPolygon();
//             if (poly.vertices.size() >= 3) {
//                 DrawTriangle(
//                     Vector2{poly.vertices[0].x, poly.vertices[0].y},
//                     Vector2{poly.vertices[2].x, poly.vertices[2].y},
//                     Vector2{poly.vertices[1].x, poly.vertices[1].y},
//                     YELLOW
//                 );
//                 // Outline
//                 for (size_t i = 0; i < poly.vertices.size(); ++i) {
//                     Vector2D p1 = poly.vertices[i];
//                     Vector2D p2 = poly.vertices[(i + 1) % poly.vertices.size()];
//                     DrawLineV(Vector2{p1.x, p1.y}, Vector2{p2.x, p2.y}, BLACK);
//                 }
//             }
//         }
//
//         // Draw dynamic capsule using transform and RLGL matrix pushes (matching your reference snippet)
//         auto capBody = dynamic_pointer_cast<CapsuleRigidBody>(capsule);
//         if (capBody) {
//             Physics::Transform t = capBody->GetTransform();
//             float height = capBody->GetHeight();
//             float radius = capBody->GetRadius();
//
//             float width = radius * 2.0f;
//             float halfHeight = height / 2.0f;
//             float halfSegment = std::max(0.0f, halfHeight - radius);
//             float rotationDeg = t.rotation * RAD2DEG;
//
//             rlPushMatrix();
//             rlTranslatef(t.position.x, t.position.y, 0.0f);
//             rlRotatef(rotationDeg, 0.0f, 0.0f, 1.0f);
//
//             Color capColor = MAROON;
//             // Draw central rectangle of the pill
//             DrawRectangleRec(
//                 Rectangle{-width / 2.0f, -halfSegment, width, halfSegment * 2.0f},
//                 capColor
//             );
//             // Draw top and bottom hemispherical end caps
//             DrawCircle(0.0f, -halfSegment, radius, capColor);
//             DrawCircle(0.0f, halfSegment, radius, capColor);
//
//             rlPopMatrix();
//         }
//
//         for (size_t i = 0; i < controlPoints.size() - 1; ++i) {
//             DrawLineV(
//                 Vector2{controlPoints[i].x, controlPoints[i].y},
//                 Vector2{controlPoints[i + 1].x, controlPoints[i + 1].y},
//                 DARKGRAY
//             );
//         }
//
//         EndDrawing();
//     }
//
//     CloseWindow();
//     return 0;
// }

#include "app/App.hpp"

int main() {
    const int windowWidth = 800;
    const int windowHeight = 600;
    const std::string windowTitle = "Bad Piggies Clone";

    App app(windowWidth, windowHeight, windowTitle);
    app.Run();
    return 0;
}
