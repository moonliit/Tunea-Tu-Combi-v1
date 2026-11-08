#include "physics/engine/RigidBodies.hpp"
using namespace std;
using namespace Math;
using namespace Physics;

BoxRigidBody::BoxRigidBody(Vector2D size)
    : m_size(size)
{
    // Initialization code for BoxRigidBody
}
BoxRigidBody::~BoxRigidBody() {
    // Cleanup code for BoxRigidBody
}

CircleRigidBody::CircleRigidBody(float radius)
    : m_radius(radius)
{
    // Initialization code for CircleRigidBody
}

CircleRigidBody::~CircleRigidBody() {
    // Cleanup code for CircleRigidBody
}

CapsuleRigidBody::CapsuleRigidBody(float height, float radius)
    : m_height(height), m_radius(radius)
{
    // Initialization code for CapsuleRigidBody
}

CapsuleRigidBody::~CapsuleRigidBody() {
    // Cleanup code for CapsuleRigidBody
}

TriangleRigidBody::TriangleRigidBody(Vector2D size)
    : m_size(size)
{
     // Initialization code for TriangleRigidBody
}

TriangleRigidBody::~TriangleRigidBody() {
     // Cleanup code for TriangleRigidBody
}
