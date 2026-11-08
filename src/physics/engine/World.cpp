#include "physics/engine/World.hpp"
using namespace std;
using namespace Math;
using namespace Physics;

World::World(Math::Vector2D gravity, int subStepCount):
    m_gravity(gravity), m_subStepCount(subStepCount) {}

void World::Step(float deltaTime) {

}
