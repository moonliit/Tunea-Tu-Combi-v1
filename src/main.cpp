// #include "raylib.h"
// #include "physics/engine/World.hpp"
// #include "physics/I_Physics.hpp"
// #include "physics/engine/RigidBodies.hpp"
// #include <vector>
// #include <memory>
//
// using namespace std;
// using namespace Math;
// using namespace Physics;
//
// int main() {
//     const int screenWidth = 800;
//     const int screenHeight = 600;
//     InitWindow(screenWidth, screenHeight, "Physics World Test - Box & Circle");
//     SetTargetFPS(60);
//
//     // Initialize World with gravity, substeps, and solver iterations
//     // gravity = {0, 980}, substeps = 4, velocityiters = 8, positioniters = 3
//     World world(Vector2D(0.0f, 980.0f), 4, 8, 3);
//
//     // Create a static terrain chain at the bottom
//     vector<Vector2D> controlPoints = {
//         Vector2D(50.0f, 500.0f),
//         Vector2D(400.0f, 530.0f),
//         Vector2D(750.0f, 500.0f)
//     };
//     world.CreateTerrainChain(controlPoints, false);
//
//     // Create a dynamic box above the slope
//     auto box = world.CreateBox(
//         Vector2D(350.0f, 200.0f),
//         0.2f,                  // slight initial rotation
//         Vector2D(50.0f, 40.0f), // size (width, height)
//         10.0f,                 // mass
//         BodyType::Dynamic
//     );
//
//     // Create a dynamic circle that will roll or drop onto the box/terrain
//     auto circle = world.CreateCircle(
//         Vector2D(500.0f, 100.0f),
//         22.0f,                 // radius
//         8.0f,                  // mass
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
//         // Draw dynamic box by downcasting to BoxRigidBody to get its world vertices
//         auto boxBody = dynamic_pointer_cast<BoxRigidBody>(box);
//         if (boxBody) {
//             // BoxRigidBody has 4 world vertices if you use GetCollisionPolygon(), or you can transform its local corners:
//             auto poly = boxBody->GetCollisionPolygon();
//             if (poly.vertices.size() >= 4) {
//                 DrawTriangle(
//                     Vector2{poly.vertices[0].x, poly.vertices[0].y},
//                     Vector2{poly.vertices[1].x, poly.vertices[1].y},
//                     Vector2{poly.vertices[2].x, poly.vertices[2].y},
//                     RED
//                 );
//                 DrawTriangle(
//                     Vector2{poly.vertices[0].x, poly.vertices[0].y},
//                     Vector2{poly.vertices[2].x, poly.vertices[2].y},
//                     Vector2{poly.vertices[3].x, poly.vertices[3].y},
//                     RED
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
//         // Draw dynamic circle
//         auto circBody = dynamic_pointer_cast<CircleRigidBody>(circle);
//         if (circBody) {
//             Vector2D cPos = circBody->GetTransform().position;
//             DrawCircleV(Vector2{cPos.x, cPos.y}, circBody->GetRadius(), DARKBLUE);
//             DrawCircleLines(Vector2{cPos.x, cPos.y}.x, Vector2{cPos.x, cPos.y}.y, circBody->GetRadius(), BLACK);
//         }
//
//         // Note: Terrain chain edges are automatically stepped and managed inside World,
//         // you can also render them if your World exposes them or via custom ground lines.
//         for (size_t i = 0; i < controlPoints.size() - 1; ++i) {
//             Vector2D p1 = controlPoints[i];
//             Vector2D p2 = controlPoints[i + 1];
//             DrawLineV(Vector2{p1.x, p1.y}, Vector2{p2.x, p2.y}, DARKGRAY);
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
