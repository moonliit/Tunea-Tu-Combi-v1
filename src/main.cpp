#include <raylib-cpp/raylib-cpp.hpp>

int main() {
    // RAII Window initialization (Closes automatically on scope exit)
    const int screenWidth = 800;
    const int screenHeight = 450;
    raylib::Window window(screenWidth, screenHeight, "Example raylib window");

    SetTargetFPS(60);

    // Vector math with built-in C++ operator overloads
    raylib::Vector2 position(400.0f, 225.0f);
    raylib::Vector2 velocity(200.0f, 150.0f);

    while (!window.ShouldClose()) {
        float deltaTime = GetFrameTime();

        // Update physics using operators
        position += velocity * deltaTime;

        // Bounce off screen boundaries
        if (position.x >= screenWidth || position.x <= 0) velocity.x *= -1.0f;
        if (position.y >= screenHeight || position.y <= 0) velocity.y *= -1.0f;

        // Render loop
        BeginDrawing();
        {
            window.ClearBackground(RAYWHITE);

            // Draw method calls on object instances
            position.DrawCircle(15.0f, MAROON);

            raylib::DrawText("Hello raylib-cpp!", 10, 10, 20, DARKGRAY);
        }
        EndDrawing();
    }

    return 0;
}
