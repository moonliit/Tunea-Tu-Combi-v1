#include "physics/Box2DPhysics.hpp"
#include "id.h"
#include <box2d/box2d.h>
#include <box2d/types.h>

// --- RigidBody Implementation ---
Box2DRigidBody::Box2DRigidBody(b2BodyId bodyId, Vector2D size)
    : m_bodyId(bodyId), m_isCircle(false), m_size(size), m_radius(0.0f) {}

Box2DRigidBody::Box2DRigidBody(b2BodyId bodyId, float radius)
    : m_bodyId(bodyId), m_isCircle(true), m_size{radius * 2.0f, radius * 2.0f}, m_radius(radius) {}

Vector2D Box2DRigidBody::GetPosition() const {
    b2Vec2 pos = b2Body_GetPosition(m_bodyId);
    return Vector2D{ pos.x, pos.y };
}

float Box2DRigidBody::GetRotation() const {
    // Box2D 3.x returns rotation as a b2Rot (cosine/sine pair), get angle in radians via b2Rot_GetAngle or b2Rot
    b2Rot rot = b2Body_GetRotation(m_bodyId);
    return b2Rot_GetAngle(rot);
}

// --- Joint Implementation ---
Box2DJoint::Box2DJoint(b2JointId jointId) : m_jointId(jointId) {}

// --- World Implementation ---
Box2DWorld::Box2DWorld(Vector2D gravity, int subStepCount) {
    b2WorldDef worldDef = b2DefaultWorldDef();
    worldDef.gravity = {
        static_cast<float>(gravity.x),
        static_cast<float>(gravity.y)
    };
    m_worldId = b2CreateWorld(&worldDef);
}

Box2DWorld::~Box2DWorld() {
    if (B2_IS_NON_NULL(m_worldId)) {
        b2DestroyWorld(m_worldId); // Destroys all bodies, shapes, and joints automatically
    }
}

void Box2DWorld::Step(float deltaTime) {
    // Box2D v3 uses subStepCount instead of velocity/position iteration counts
    float timeStep = deltaTime > 0.0f ? deltaTime : (1.0f / 60.0f);
    b2World_Step(m_worldId, timeStep, 4);
}

std::shared_ptr<IRigidBody> Box2DWorld::CreateBox(Vector2D position, float rotation, Vector2D size, float mass, BodyType type) {
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

    return std::make_shared<Box2DRigidBody>(bodyId, size);
}

std::shared_ptr<IRigidBody> Box2DWorld::CreateCircle(Vector2D position, float radius, float mass, BodyType type) {
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

    return std::make_shared<Box2DRigidBody>(bodyId, radius);
}

std::shared_ptr<IJoint> Box2DWorld::CreateRevoluteJoint(
    std::shared_ptr<IRigidBody> bodyA,
    std::shared_ptr<IRigidBody> bodyB,
    Vector2D anchor
) {
    auto bA = std::dynamic_pointer_cast<Box2DRigidBody>(bodyA);
    auto bB = std::dynamic_pointer_cast<Box2DRigidBody>(bodyB);

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
    return std::make_shared<Box2DJoint>(jointId);
}

// Función auxiliar para evaluar un punto en un segmento de Catmull-Rom
Vector2D EvaluateCatmullRom(Vector2D p0, Vector2D p1, Vector2D p2, Vector2D p3, float t) {
    float t2 = t * t;
    float t3 = t2 * t;

    float x = 0.5f * ((2.0f * p1.x) +
                      (-p0.x + p2.x) * t +
                      (2.0f * p0.x - 5.0f * p1.x + 4.0f * p2.x - p3.x) * t2 +
                      (-p0.x + 3.0f * p1.x - 3.0f * p2.x + p3.x) * t3);

    float y = 0.5f * ((2.0f * p1.y) +
                      (-p0.y + p2.y) * t +
                      (2.0f * p0.y - 5.0f * p1.y + 4.0f * p2.y - p3.y) * t2 +
                      (-p0.y + 3.0f * p1.y - 3.0f * p2.y + p3.y) * t3);

    return {x, y};
}

std::vector<b2Vec2> SampleTerrain(const std::vector<Vector2D>& controlPoints, int resolution) {
    std::vector<b2Vec2> sampledVertices;
    if (controlPoints.size() < 2) return sampledVertices;

    // Para Catmull-Rom necesitamos duplicar o extender los puntos extremos
    // para que el primer y último tramo tengan vecinos válidos (p0 y p3).
    std::vector<Vector2D> extendedPoints = controlPoints;

    // Duplicar el inicio y el final para manejar los extremos con gracia
    extendedPoints.insert(extendedPoints.begin(), controlPoints.front());
    extendedPoints.push_back(controlPoints.back());

    for (size_t i = 1; i < extendedPoints.size() - 2; ++i) {
        Vector2D p0 = extendedPoints[i - 1];
        Vector2D p1 = extendedPoints[i];
        Vector2D p2 = extendedPoints[i + 1];
        Vector2D p3 = extendedPoints[i + 2];

        for (int j = 0; j < resolution; ++j) {
            float t = static_cast<float>(j) / static_cast<float>(resolution);
            Vector2D evaluated = EvaluateCatmullRom(p0, p1, p2, p3, t);
            sampledVertices.push_back({
                static_cast<float>(evaluated.x),
                static_cast<float>(evaluated.y),
            });
        }
    }

    // Añadir el punto final exacto
    sampledVertices.push_back({
        static_cast<float>(controlPoints.back().x),
        static_cast<float>(controlPoints.back().y),
    });
    return sampledVertices;
}

std::vector<Vector2D> Box2DWorld::CreateTerrainChain(const std::vector<Vector2D>& controlPoints, int resolutionPerSegment) {
    // 1. Sample control points into a smooth vertex chain
    std::vector<b2Vec2> vertices = SampleTerrain(controlPoints, resolutionPerSegment);
    if (vertices.size() < 2) return {};

    // 2. Create static body for the terrain
    b2BodyDef bodyDef = b2DefaultBodyDef();
    bodyDef.type = b2_staticBody;
    bodyDef.position = {0.0f, 0.0f};
    b2BodyId bodyId = b2CreateBody(m_worldId, &bodyDef);

    // 3. Configure the chain definition using Box2D v3 fields
    b2ChainDef chainDef = b2DefaultChainDef();
    chainDef.points = vertices.data();
    chainDef.pointCount = static_cast<int>(vertices.size());
    chainDef.isLoop = false;

    // 4. Compute valid ghost points to prevent NaN/inf errors in open chains.
    // Ghost 1: Extrapolate backwards from the first two points
    b2Vec2 p0 = vertices[0];
    b2Vec2 p1 = vertices[1];
    chainDef.ghost1 = {p0.x - (p1.x - p0.x), p0.y - (p1.y - p0.y)};

    // Ghost 2: Extrapolate forwards from the last two points
    b2Vec2 pn_1 = vertices[vertices.size() - 2];
    b2Vec2 pn = vertices.back();
    chainDef.ghost2 = {pn.x + (pn.x - pn_1.x), pn.y + (pn.y - pn_1.y)};

    // Set surface material properties (like friction) via b2SurfaceMaterial
    b2SurfaceMaterial material = b2DefaultSurfaceMaterial();
    material.friction = 0.6f;
    chainDef.materials = &material;
    chainDef.materialCount = 1;

    // Create the chain shape on the body
    b2CreateChain(bodyId, &chainDef);

    // 4. Convert b2Vec2 back to Vector2D and return for rendering
    std::vector<Vector2D> resultPoints;
    resultPoints.reserve(vertices.size());
    for (const auto& v : vertices) {
        resultPoints.push_back({v.x, v.y});
    }
    return resultPoints;
}
