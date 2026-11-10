#include "physics/engine/RigidBodies.hpp"
#include "math/Vector2D.hpp"
#include "physics/engine/common/CollisionPolygon.hpp"
using namespace std;
using namespace Math;
using namespace Physics;

BoxRigidBody::BoxRigidBody(
    Vector2D position,
    RealNum rotation,
    Vector2D size,
    RealNum mass,
    BodyType type,
    Material material
): m_size(size) {
    m_transform.position = position;
    m_transform.rotation = rotation;

    m_massData.mass = (type == BodyType::Static) ? RealNum(0) : mass;
    m_massData.invMass = (m_massData.mass > 0.0f) ? (RealNum(1) / m_massData.mass) : RealNum(0);

    m_bodyType = type;
    m_material = material;

    if (m_massData.mass > 0.0f) {
        // moment of inertia for a box: 1/12 * m * (w^2 + h^2)
        m_massData.inertia = (1.0f / 12.0f) * m_massData.mass * (m_size.x * m_size.x + m_size.y * m_size.y);
        m_massData.invInertia = 1.0f / m_massData.inertia;
    } else {
        m_massData.inertia = 0.0f;
        m_massData.invInertia = 0.0f;
    }

    UpdateAABB();
}

CollisionPolygon BoxRigidBody::GetCollisionPolygon() const {
    RealNum halfW = m_size.x * 0.5f;
    RealNum halfH = m_size.y * 0.5f;

    vector<Vector2D> localVertices = {
        { -halfW, -halfH }, // Bottom-left
        {  halfW, -halfH }, // Bottom-right
        {  halfW,  halfH }, // Top-right
        { -halfW,  halfH }  // Top-left
    };

    vector<Vector2D> worldVertices(localVertices.size());
    const auto& transform = GetTransform();
    for (size_t i = 0; i < localVertices.size(); ++i) {
        worldVertices[i] = transform.ToWorldPoint(localVertices[i]);
    }
    return CollisionPolygon { worldVertices };
}

void BoxRigidBody::UpdateAABB() {
    float halfWidth = m_size.x / 2.0f;
    float halfHeight = m_size.y / 2.0f;

    Vector2D localCorners[4] = {
        { -halfWidth, -halfHeight },
        {  halfWidth, -halfHeight },
        {  halfWidth,  halfHeight },
        { -halfWidth,  halfHeight }
    };

    Vector2D firstCorner = m_transform.ToWorldPoint(localCorners[0]);
    Vector2D minPoint = firstCorner;
    Vector2D maxPoint = firstCorner;

    for (int i = 0; i < 4; ++i) {
        Vector2D worldCorner = m_transform.ToWorldPoint(localCorners[i]);
        minPoint.x = min(minPoint.x, worldCorner.x);
        minPoint.y = min(minPoint.y, worldCorner.y);
        maxPoint.x = max(maxPoint.x, worldCorner.x);
        maxPoint.y = max(maxPoint.y, worldCorner.y);
    }

    m_aabb.min = minPoint;
    m_aabb.max = maxPoint;
}

CircleRigidBody::CircleRigidBody(
    Vector2D position,
    RealNum radius,
    RealNum mass,
    BodyType type,
    Material material
): m_radius(radius) {
    m_transform.position = position;
    m_transform.rotation = 0.0f;

    m_massData.mass = (type == BodyType::Static) ? RealNum(0) : mass;
    m_massData.invMass = (m_massData.mass > 0.0f) ? (RealNum(1) / m_massData.mass) : RealNum(0);

    m_bodyType = type;
    m_material = material;

    if (m_massData.mass > 0.0f) {
        // moment of inertia for a circle: 1/2 * m * r^2
        m_massData.inertia = 0.5f * m_massData.mass * m_radius * m_radius;
        m_massData.invInertia = 1.0f / m_massData.inertia;
    } else {
        m_massData.inertia = 0.0f;
        m_massData.invInertia = 0.0f;
    }

    UpdateAABB();
}

void CircleRigidBody::UpdateAABB() {
    Vector2D radiusVector = { m_radius, m_radius };
    m_aabb.min = m_transform.position - radiusVector;
    m_aabb.max = m_transform.position + radiusVector;
}

CapsuleRigidBody::CapsuleRigidBody(
    Vector2D position,
    RealNum rotation,
    RealNum height,
    RealNum radius,
    RealNum mass,
    BodyType type,
    Material material
): m_height(height), m_radius(radius) {
    m_transform.position = position;
    m_transform.rotation = rotation;

    m_massData.mass = (type == BodyType::Static) ? RealNum(0) : mass;
    m_massData.invMass = (m_massData.mass > 0.0f) ? (RealNum(1) / m_massData.mass) : RealNum(0);

    m_bodyType = type;
    m_material = material;

    if (m_massData.mass > 0.0f) {
        // approximation for capsule inertia (similar to elongated cylinder + caps)
        RealNum totalH = m_height + 2.0f * m_radius;
        m_massData.inertia = (1.0f / 12.0f) * m_massData.mass * (totalH * totalH + 3.0f * m_radius * m_radius);
        m_massData.invInertia = 1.0f / m_massData.inertia;
    } else {
        m_massData.inertia = 0.0f;
        m_massData.invInertia = 0.0f;
    }

    UpdateAABB();
}

void CapsuleRigidBody::UpdateAABB() {
    float halfHeight = m_height * 0.5f;
    Vector2D localTop(0.0f, halfHeight);
    Vector2D localBottom(0.0f, -halfHeight);

    Vector2D worldTop = m_transform.ToWorldPoint(localTop);
    Vector2D worldBottom = m_transform.ToWorldPoint(localBottom);

    m_aabb.min = Vector2D(
        min(worldTop.x, worldBottom.x) - m_radius,
        min(worldTop.y, worldBottom.y) - m_radius
    );
    m_aabb.max = Vector2D(
        max(worldTop.x, worldBottom.x) + m_radius,
        max(worldTop.y, worldBottom.y) + m_radius
    );
}

TriangleRigidBody::TriangleRigidBody(
    Vector2D position,
    RealNum rotation,
    Vector2D size,
    RealNum mass,
    BodyType type,
    Material material
): m_size(size) {
    m_transform.position = position;
    m_transform.rotation = rotation;
    m_massData.mass = (type == BodyType::Static) ? RealNum(0) : mass;
    m_massData.invMass = (m_massData.mass > 0.0f) ? (RealNum(1) / m_massData.mass) : RealNum(0);

    m_bodyType = type;
    m_material = material;

    if (m_massData.mass > 0.0f) {
        // empiric approximation of triangle inertia using a smaller fraction of a box inertia
        m_massData.inertia = (1.0f / 18.0f) * m_massData.mass * (m_size.x * m_size.x + m_size.y * m_size.y);
        m_massData.invInertia = 1.0f / m_massData.inertia;
    } else {
        m_massData.inertia = 0.0f;
        m_massData.invInertia = 0.0f;
    }

    UpdateAABB();
}

CollisionPolygon TriangleRigidBody::GetCollisionPolygon() const {
    RealNum halfW = m_size.x * 0.5f;
    RealNum halfH = m_size.y * 0.5f;

    Vector2D localVertices[3] = {
        { -halfW,      0 }, // Vertex 0
        {  halfW, -halfH }, // Vertex 1 (bottom left)
        {  halfW,  halfH }  // Vertex 2 (bottom right)
    };

    vector<Vector2D> worldVertices(3);
    const auto& transform = GetTransform();
    for (int i = 0; i < 3; ++i) {
        worldVertices[i] = transform.ToWorldPoint(localVertices[i]);
    }
    return CollisionPolygon { worldVertices };
}

void TriangleRigidBody::UpdateAABB() {
    vector<Vector2D> worldVertices = GetCollisionPolygon().vertices;

    Vector2D firstVert = worldVertices[0];
    Vector2D minCoord = firstVert;
    Vector2D maxCoord = firstVert;

    for (int i = 0; i < 3; ++i) {
        Vector2D worldVert = worldVertices[i];
        minCoord.x = min(minCoord.x, worldVert.x);
        minCoord.y = min(minCoord.y, worldVert.y);
        maxCoord.x = max(maxCoord.x, worldVert.x);
        maxCoord.y = max(maxCoord.y, worldVert.y);
    }

    m_aabb.min = minCoord;
    m_aabb.max = maxCoord;
}

EdgeRigidBody::EdgeRigidBody(Math::Vector2D p1, Math::Vector2D p2)
    : m_localPoint1(p1), m_localPoint2(p2)
{
    m_bodyType = BodyType::Static;
    UpdateAABB();
}

void EdgeRigidBody::UpdateAABB() {
    m_worldPoint1 = m_transform.ToWorldPoint(m_localPoint1);
    m_worldPoint2 = m_transform.ToWorldPoint(m_localPoint2);

    // outward normal
    Vector2D delta = m_worldPoint2 - m_worldPoint1;
    m_normal = Vector2D(-delta.y, delta.x).Normal().value_or(Vector2D::Up());

    // AABB enclosing both points
    float minX = min(m_worldPoint1.x, m_worldPoint2.x);
    float maxX = max(m_worldPoint1.x, m_worldPoint2.x);
    float minY = min(m_worldPoint1.y, m_worldPoint2.y);
    float maxY = max(m_worldPoint1.y, m_worldPoint2.y);

    m_aabb = Math::AABB(Vector2D(minX, minY), Vector2D(maxX, maxY));
}
