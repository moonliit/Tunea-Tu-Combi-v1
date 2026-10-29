#include "physics/Box2D_Physics.hpp"
#include "id.h"
#include "physics/I_Physics.hpp"
#include <box2d/box2d.h>
#include <box2d/types.h>
#include <float.h>

using namespace Math;


Vector2D Box2D_IRigidBody::GetPosition() const {
    b2BodyId bodyId = GetInternalId();
    b2Vec2 pos = b2Body_GetPosition(bodyId);
    return Math::Vector2D{ pos.x, pos.y };
}

RealNum Box2D_IRigidBody::GetRotation() const {
    b2BodyId bodyId = GetInternalId();
    b2Rot rot = b2Body_GetRotation(bodyId);
    return Math::RealNum {b2Rot_GetAngle(rot)};
}

// --- Box RigidBody Implementation ---
Box2D_BoxRigidBody::Box2D_BoxRigidBody(b2BodyId bodyId, Math::Vector2D size)
    : m_bodyId(bodyId), m_size(size) {}

Box2D_BoxRigidBody::~Box2D_BoxRigidBody() {
    if (b2Body_IsValid(m_bodyId)) {
        b2DestroyBody(m_bodyId);
    }
}

// --- Circle RigidBody Implementation ---
Box2D_CircleRigidBody::Box2D_CircleRigidBody(b2BodyId bodyId, float radius)
    : m_bodyId(bodyId), m_radius(radius) {}

Box2D_CircleRigidBody::~Box2D_CircleRigidBody() {
    if (b2Body_IsValid(m_bodyId)) {
        b2DestroyBody(m_bodyId);
    }
}

// --- Capsule RigidBody Implementation ---
Box2D_CapsuleRigidBody::Box2D_CapsuleRigidBody(b2BodyId bodyId, float height, float radius)
    : m_bodyId(bodyId), m_height(height), m_radius(radius) {}

Box2D_CapsuleRigidBody::~Box2D_CapsuleRigidBody() {
    if (b2Body_IsValid(m_bodyId)) {
        b2DestroyBody(m_bodyId);
    }
}

// --- Triangle RigidBody Implementation ---
Box2D_TriangleRigidBody::Box2D_TriangleRigidBody(b2BodyId bodyId, Math::Vector2D size)
    : m_bodyId(bodyId), m_size(size) {}

Box2D_TriangleRigidBody::~Box2D_TriangleRigidBody() {
    if (b2Body_IsValid(m_bodyId)) {
        b2DestroyBody(m_bodyId);
    }
}

// --- Joint Implementation ---
Box2D_Joint::Box2D_Joint(b2JointId jointId) : m_jointId(jointId) {}

// --- World Implementation ---
Box2D_PhysicsWorld::Box2D_PhysicsWorld(Math::Vector2D gravity, int subStepCount) {
    b2WorldDef worldDef = b2DefaultWorldDef();
    worldDef.gravity = {
        static_cast<float>(gravity.x),
        static_cast<float>(gravity.y)
    };
    m_worldId = b2CreateWorld(&worldDef);
}

Box2D_PhysicsWorld::~Box2D_PhysicsWorld() {
    if (B2_IS_NON_NULL(m_worldId)) {
        b2DestroyWorld(m_worldId); // Destroys all bodies, shapes, and joints automatically
    }
}

void Box2D_PhysicsWorld::Step(float deltaTime) {
    // Box2D v3 uses subStepCount instead of velocity/position iteration counts
    float timeStep = deltaTime > 0.0f ? deltaTime : (1.0f / 60.0f);
    b2World_Step(m_worldId, timeStep, 4);
}

std::shared_ptr<I_RigidBody> Box2D_PhysicsWorld::CreateBox(
    Math::Vector2D position, float rotation, Math::Vector2D size, float mass, BodyType type
) {
    b2BodyDef bodyDef = b2DefaultBodyDef();
    bodyDef.type = (type == BodyType::Static) ? b2_staticBody : b2_dynamicBody;
    bodyDef.position = {
        static_cast<float>(position.x),
        static_cast<float>(position.y)
    };
    bodyDef.rotation = b2MakeRot(rotation);

    b2BodyId bodyId = b2CreateBody(m_worldId, &bodyDef);

    // Box2D 3.x shape definitions
    b2Polygon boxPolygon = b2MakeBox(size.x / 2.0f, size.y / 2.0f);

    b2ShapeDef shapeDef = b2DefaultShapeDef();
    if (type == BodyType::Static) {
        shapeDef.density = 0.0f;
    } else {
        shapeDef.density = (mass > 0.0f) ? mass / (size.x * size.y) : 1.0f;
    }
    shapeDef.material.friction = 0.6f;

    b2CreatePolygonShape(bodyId, &shapeDef, &boxPolygon);

    return std::make_shared<Box2D_BoxRigidBody>(bodyId, size);
}

std::shared_ptr<I_RigidBody> Box2D_PhysicsWorld::CreateCircle(
    Math::Vector2D position, float radius, float mass, BodyType type
) {
    b2BodyDef bodyDef = b2DefaultBodyDef();
    bodyDef.type = (type == BodyType::Static) ? b2_staticBody : b2_dynamicBody;
    bodyDef.position = {
        static_cast<float>(position.x),
        static_cast<float>(position.y)
    };

    // Add damping to simulate air resistance and rolling energy loss
    if (type != BodyType::Static) {
        bodyDef.angularDamping = 0.5f; // Adjust this value (e.g., 0.1 to 2.0) to control how fast it stops rolling
        bodyDef.linearDamping = 0.1f;   // Optional: adds slight air drag
    }

    b2BodyId bodyId = b2CreateBody(m_worldId, &bodyDef);

    b2Circle circle;
    circle.radius = radius;
    circle.center = {0.0f, 0.0f};

    b2ShapeDef shapeDef = b2DefaultShapeDef();
    if (type == BodyType::Static) {
        shapeDef.density = 0.0f;
    } else {
        float area = B2_PI * radius * radius;
        shapeDef.density = (mass > 0.0f) ? mass / area : 1.0f;
    }
    shapeDef.material.friction = 0.6f;

    b2CreateCircleShape(bodyId, &shapeDef, &circle);

    return std::make_shared<Box2D_CircleRigidBody>(bodyId, radius);
}

std::shared_ptr<I_RigidBody> Box2D_PhysicsWorld::CreateCapsule(
    Math::Vector2D position, float rotation, float height, float radius, float mass, BodyType type
) {
    b2BodyDef bodyDef = b2DefaultBodyDef();
    bodyDef.type = (type == BodyType::Static) ? b2_staticBody : b2_dynamicBody;
    bodyDef.position = { static_cast<float>(position.x), static_cast<float>(position.y) };
    bodyDef.rotation = b2MakeRot(rotation);

    b2BodyId bodyId = b2CreateBody(m_worldId, &bodyDef);

    // Compute half-height of the linear segment between the two circle centers
    float halfSegment = std::max(0.0f, (height * 0.5f) - radius);

    b2Capsule capsule;
    capsule.center1 = { 0.0f, -halfSegment };
    capsule.center2 = { 0.0f,  halfSegment };
    capsule.radius  = radius;

    b2ShapeDef shapeDef = b2DefaultShapeDef();
    if (type == BodyType::Static) {
        shapeDef.density = 0.0f;
    } else {
        // Approximate capsule area: two semi-circles (= 1 full circle) + rectangular body
        float area = (B2_PI * radius * radius) + (2.0f * radius * (2.0f * halfSegment));
        shapeDef.density = (mass > 0.0f) ? mass / area : 1.0f;
    }
    shapeDef.material.friction = 0.6f;

    b2CreateCapsuleShape(bodyId, &shapeDef, &capsule);

    return std::make_shared<Box2D_CapsuleRigidBody>(bodyId, height, radius);
}

#include <print>

std::shared_ptr<I_RigidBody> Box2D_PhysicsWorld::CreateTriangle(
    Math::Vector2D position, float rotation, Math::Vector2D size, float mass, BodyType type
) {
    b2BodyDef bodyDef = b2DefaultBodyDef();
    bodyDef.type = (type == BodyType::Static) ? b2_staticBody : b2_dynamicBody;
    bodyDef.position = { static_cast<float>(position.x), static_cast<float>(position.y) };
    bodyDef.rotation = b2MakeRot(rotation);

    b2BodyId bodyId = b2CreateBody(m_worldId, &bodyDef);

    // Define vertices in counter-clockwise order
    b2Vec2 points[3] = {
        { 0.0f, -size.y * 0.5f },             // Top tip
        { -size.x * 0.5f,  size.y * 0.5f },    // Bottom-left
        {  size.x * 0.5f,  size.y * 0.5f }     // Bottom-right
    };

    // Use Box2D's official hull computation helper to ensure valid geometry and winding order
    b2Hull hull = b2ComputeHull(points, 3);
    b2Polygon triangle = b2MakePolygon(&hull, 0.0f); // 0.0f radius

    b2ShapeDef shapeDef = b2DefaultShapeDef();
    if (type == BodyType::Static) {
        shapeDef.density = 0.0f;
    } else {
        float area = 0.5f * size.x * size.y;
        shapeDef.density = (mass > 0.0f) ? mass / area : 1.0f;
    }
    shapeDef.material.friction = 0.6f;

    b2CreatePolygonShape(bodyId, &shapeDef, &triangle);

    return std::make_shared<Box2D_TriangleRigidBody>(bodyId, size);
}

std::shared_ptr<I_Joint> Box2D_PhysicsWorld::CreateRevoluteJoint(
    std::shared_ptr<I_RigidBody> bodyA,
    std::shared_ptr<I_RigidBody> bodyB,
    Math::Vector2D anchor
) {
    auto bA = std::dynamic_pointer_cast<Box2D_IRigidBody>(bodyA);
    auto bB = std::dynamic_pointer_cast<Box2D_IRigidBody>(bodyB);

    if (!bA || !bB) return nullptr;

    b2BodyId idA = bA->GetInternalId();
    b2BodyId idB = bB->GetInternalId();

    b2RevoluteJointDef jointDef = b2DefaultRevoluteJointDef();
    jointDef.base.bodyIdA = idA;
    jointDef.base.bodyIdB = idB;

    // In v3, you can define local anchors or use helper initializations depending on world anchor translation
    // Here we define the world anchor point directly:
    b2Vec2 worldAnchor = {
        static_cast<float>(anchor.x),
        static_cast<float>(anchor.y)
    };
    jointDef.base.localFrameA.p = b2Body_GetLocalPoint(idA, worldAnchor);
    jointDef.base.localFrameB.p = b2Body_GetLocalPoint(idB, worldAnchor);

    jointDef.base.collideConnected = false;

    b2JointId jointId = b2CreateRevoluteJoint(m_worldId, &jointDef);
    return std::make_shared<Box2D_Joint>(jointId);
}

std::vector<Math::Vector2D> Box2D_PhysicsWorld::CreateTerrainChain(const std::vector<Math::Vector2D>& catmullRomPoints) {
    // 1. Get pure Math::Vector2D sampled points from decoupled math module
    if (catmullRomPoints.size() < 2) return {};

    // 2. Convert to Box2D's b2Vec2 for internal physics creation
    std::vector<b2Vec2> b2Vertices;
    b2Vertices.reserve(catmullRomPoints.size());
    for (const auto& v : catmullRomPoints) {
        b2Vertices.push_back({static_cast<float>(v.x), static_cast<float>(v.y)});
    }

    // 3. Create static body for terrain
    b2BodyDef bodyDef = b2DefaultBodyDef();
    bodyDef.type = b2_staticBody;
    bodyDef.position = {0.0f, 0.0f};
    b2BodyId bodyId = b2CreateBody(m_worldId, &bodyDef);

    // 4. Configure chain definition
    b2ChainDef chainDef = b2DefaultChainDef();
    chainDef.points = b2Vertices.data();
    chainDef.pointCount = static_cast<int>(b2Vertices.size());
    chainDef.isLoop = false;

    // Ghost points for smooth sliding
    b2Vec2 p0 = b2Vertices[0];
    b2Vec2 p1 = b2Vertices[1];
    chainDef.ghost1 = {p0.x - (p1.x - p0.x), p0.y - (p1.y - p0.y)};

    b2Vec2 pn_1 = b2Vertices[b2Vertices.size() - 2];
    b2Vec2 pn = b2Vertices.back();
    chainDef.ghost2 = {pn.x + (pn.x - pn_1.x), pn.y + (pn.y - pn_1.y)};

    b2SurfaceMaterial material = b2DefaultSurfaceMaterial();
    material.friction = 0.6f;
    chainDef.materials = &material;
    chainDef.materialCount = 1;

    b2CreateChain(bodyId, &chainDef);

    // Return the exact same pure vertices so rendering matches physics 1:1
    return catmullRomPoints;
}
