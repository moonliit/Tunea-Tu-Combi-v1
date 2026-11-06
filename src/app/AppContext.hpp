#ifndef APP_CONTEXT_HPP
#define APP_CONTEXT_HPP

#include <string>
#include "app/SceneManager.hpp"

struct AppContext {
    int windowWidth;
    int windowHeight;
    std::string windowTitle;
    SceneRequester* sceneRequester;
};

#endif
