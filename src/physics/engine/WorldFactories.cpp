#include "physics/engine/RigidBodies.hpp"
#include "physics/engine/World.hpp"
#include <memory>
using namespace std;
using namespace Math;
using namespace Physics;

using PhysMat = Physics::Material;

shared_ptr<I_RigidBody> World::CreateBox(
    Vector2D position,
    float rotation,
    Vector2D size,
    float mass,
    BodyType type
) {
    PhysMat defaultMaterial;
    auto box = make_shared<BoxRigidBody>(
         position,
         rotation,
         size,
         mass,
         type,
         defaultMaterial
    );
    m_bodies.push_back(box);
    return box;
}

shared_ptr<I_RigidBody> World::CreateCircle(
    Vector2D position,
    float radius,
    float mass,
    BodyType type
) {
    PhysMat defaultMaterial;
    auto circle = make_shared<CircleRigidBody>(
         position,
         radius,
         mass,
         type,
         defaultMaterial
    );
    m_bodies.push_back(circle);
    return circle;
}

shared_ptr<I_RigidBody> World::CreateCapsule(
    Vector2D position,
    float rotation,
    float height,
    float radius,
    float mass,
    BodyType type
) {
    PhysMat defaultMaterial;
    auto capsule = make_shared<CapsuleRigidBody>(
         position,
         rotation,
         height,
         radius,
         mass,
         type,
         defaultMaterial
    );
    m_bodies.push_back(capsule);
    return capsule;
}

shared_ptr<I_RigidBody> World::CreateTriangle(
    Vector2D position,
    float rotation,
    Vector2D size,
    float mass,
    BodyType type
) {
    PhysMat defaultMaterial;
    auto triangle = make_shared<TriangleRigidBody>(
         position,
         rotation,
         size,
         mass,
         type,
         defaultMaterial
    );
    m_bodies.push_back(triangle);
    return triangle;
}

vector<Vector2D> World::CreateTerrainChain(
    const vector<Vector2D>& controlPoints,
    bool isLoop
) {
     if (controlPoints.size() < 2) return {};

    // if segment is not looped, theres one less control point
    size_t segmentCount = controlPoints.size() - static_cast<size_t>(!isLoop);

    for (size_t i = 0; i < segmentCount; ++i) {
        Vector2D p1 = controlPoints[i];
        Vector2D p2 = controlPoints[(i + 1) % controlPoints.size()];
        auto edgeBody = make_shared<EdgeRigidBody>(p1, p2);
        m_bodies.push_back(edgeBody);
    }
    return controlPoints;
}

shared_ptr<I_Joint> World::CreateRevoluteJoint(
    shared_ptr<I_RigidBody> bodyA,
    shared_ptr<I_RigidBody> bodyB,
    Vector2D anchor
) {
    JointDef def;
    def.bodyA = dynamic_pointer_cast<BaseRigidBody>(bodyA);
    def.bodyB = dynamic_pointer_cast<BaseRigidBody>(bodyB);
    def.anchorA = anchor; // Or transform anchor into local body space if your constructor expects local anchors!
    def.anchorB = anchor;
    def.collideConnected = false;

    auto joint = make_shared<RevoluteJoint>(def);
    m_joints.push_back(joint);
    return joint;
}

shared_ptr<I_Joint> World::CreateWeldJoint(
    shared_ptr<I_RigidBody> bodyA,
    shared_ptr<I_RigidBody> bodyB,
    Vector2D anchor
) {
    JointDef def;
    def.bodyA = dynamic_pointer_cast<BaseRigidBody>(bodyA);
    def.bodyB = dynamic_pointer_cast<BaseRigidBody>(bodyB);
    def.anchorA = anchor;
    def.anchorB = anchor;
    def.collideConnected = false;

    auto joint = make_shared<WeldJoint>(def);
    m_joints.push_back(joint);
    return joint;
}
