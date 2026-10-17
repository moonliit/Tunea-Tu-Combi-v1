#include "game/Game.hpp"

// TODO: deprecate raylib-cpp and migrate to regular raylib

int main() {
    const int windowWidth = 800;
    const int windowHeight = 600;
    const std::string windowTitle = "Bad Piggies Clone - Phase 2";

    Game game(windowWidth, windowHeight, windowTitle);
    game.Run();
    return 0;
}
