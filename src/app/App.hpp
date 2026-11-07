#ifndef APP_HPP
#define APP_HPP

#include "app/SceneManager.hpp"
#include "app/AppContext.hpp"

#include <string>

class App {
private:
    SceneManager m_sceneManager;
    SceneRequester m_sceneRequester;
    AppContext m_appContext;

public:
    App(
        int width,
        int height,
        const std::string& title
    );
    ~App() = default;

    void Run();
};

#endif
