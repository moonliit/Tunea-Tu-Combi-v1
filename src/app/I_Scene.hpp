#ifndef GAME_SCENE_HPP
#define GAME_SCENE_HPP

class I_Scene {
public:
    virtual ~I_Scene() = default;
    virtual void ProcessInput(float deltaTime) = 0;
    virtual void Update(float deltaTime) = 0;
    virtual void Render() const = 0;
};

#endif
