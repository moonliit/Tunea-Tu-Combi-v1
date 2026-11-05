#ifndef APP_HPP
#define APP_HPP

#include "app/game/Level.hpp"
#include "app/SceneManager.hpp"

#include <string>

class App {
private:
    SceneManager m_sceneManager;

public:
    App(int width, int height, const std::string& title, LevelSpecs specs);
    ~App() = default;

    void Run();
};

#endif
