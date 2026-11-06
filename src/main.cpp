#include "app/App.hpp"

int main() {
    const int windowWidth = 800;
    const int windowHeight = 600;
    const std::string windowTitle = "Bad Piggies Clone";

    App app(windowWidth, windowHeight, windowTitle);
    app.Run();
    return 0;
}
