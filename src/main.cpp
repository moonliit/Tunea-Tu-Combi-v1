#include "app/App.hpp"

int main() {
    const int windowWidth = 1200;
    const int windowHeight = 720;
    const std::string windowTitle = "Bad Piggies Clone";

    App app(windowWidth, windowHeight, windowTitle);
    app.Run();
    return 0;
}
