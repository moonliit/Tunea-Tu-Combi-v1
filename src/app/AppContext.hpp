#ifndef APP_CONTEXT_HPP
#define APP_CONTEXT_HPP

#include <string>
#include <memory>
#include "app/SceneManager.hpp"

class CustomLevelCatalog;

class AudioController;
class TextureLibrary;

struct AppContext {
    int windowWidth;
    int windowHeight;
    std::string windowTitle;
    SceneRequester* sceneRequester;
    int unlockedLevel = 1;
    int completedLevel = 0;
    bool returnToCustomDesign = false;
    bool returnedToSelector = false;
    AudioController* audioController = nullptr;
    TextureLibrary* textureLibrary = nullptr;
    std::shared_ptr<CustomLevelCatalog> customLevels;
    int activeCustomLevel = -1;
    bool openCustomMusicPicker = false;
};

#endif
