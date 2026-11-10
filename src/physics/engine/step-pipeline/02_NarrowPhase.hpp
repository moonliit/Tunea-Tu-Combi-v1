#ifndef PHYSICS_NARROW_PHASE_HPP
#define PHYSICS_NARROW_PHASE_HPP

#include <memory>
#include "physics/engine/common/Common.hpp"
#include "physics/engine/BaseRigidBody.hpp"
#include "physics/engine/RigidBodies.hpp"

namespace Physics {
    class NarrowPhase {
    public:
        static bool DetectCollision(
            std::shared_ptr<BaseRigidBody> bodyA,
            std::shared_ptr<BaseRigidBody> bodyB,
            Manifold& outManifold
        ) {
            if (!bodyA || !bodyB) return false;

            outManifold.bodyA = bodyA;
            outManifold.bodyB = bodyB;
            outManifold.collisionPoints.clear();

            RbShape shapeA = bodyA->GetRbShape();
            RbShape shapeB = bodyB->GetRbShape();

            // per basis cases
            switch (shapeA) {
                case RbShape::Box: {
                    BoxRigidBody boxA = *static_pointer_cast<BoxRigidBody>(bodyA);
                    switch (shapeB) {
                        case RbShape::Box: {
                            BoxRigidBody boxB = *static_pointer_cast<BoxRigidBody>(bodyB);
                            return DetectBoxBoxCollision(boxA, boxB, outManifold);
                        }
                        case RbShape::Circle: {
                            CircleRigidBody circleB = *static_pointer_cast<CircleRigidBody>(bodyB);
                            return DetectBoxCircleCollision(boxA, circleB, outManifold);
                        }
                        case RbShape::Capsule: {
                            CapsuleRigidBody capsuleB = *static_pointer_cast<CapsuleRigidBody>(bodyB);
                            return DetectBoxCapsuleCollision(boxA, capsuleB, outManifold);
                        }
                        case RbShape::Triangle: {
                            TriangleRigidBody triangleB = *static_pointer_cast<TriangleRigidBody>(bodyB);
                            return DetectBoxTriangleCollision(boxA, triangleB, outManifold);
                        }
                        case RbShape::Edge: {
                            EdgeRigidBody edgeB = *static_pointer_cast<EdgeRigidBody>(bodyB);
                            return DetectBoxEdgeCollision(boxA, edgeB, outManifold);
                        }
                        case RbShape::TerrainMesh: {
                            TerrainMeshRigidBody terrainMeshB = *static_pointer_cast<TerrainMeshRigidBody>(bodyB);
                            return DetectBoxTerrainMeshCollision(boxA, terrainMeshB, outManifold);
                        }
                        default:
                            return false;
                    }
                }
                case RbShape::Circle: {
                    CircleRigidBody circleA = *static_pointer_cast<CircleRigidBody>(bodyA);
                    switch (shapeB) {
                        case RbShape::Box: {
                            BoxRigidBody boxB = *static_pointer_cast<BoxRigidBody>(bodyB);
                            return Invert(DetectBoxCircleCollision(boxB, circleA, outManifold), outManifold);
                        }
                        case RbShape::Circle: {
                            CircleRigidBody circleB = *static_pointer_cast<CircleRigidBody>(bodyB);
                            return DetectCircleCircleCollision(circleA, circleB, outManifold);
                        }
                        case RbShape::Capsule: {
                            CapsuleRigidBody capsuleB = *static_pointer_cast<CapsuleRigidBody>(bodyB);
                            return DetectCircleCapsuleCollision(circleA, capsuleB, outManifold);
                        }
                        case RbShape::Triangle: {
                            TriangleRigidBody triangleB = *static_pointer_cast<TriangleRigidBody>(bodyB);
                            return DetectCircleTriangleCollision(circleA, triangleB, outManifold);
                        }
                        case RbShape::Edge: {
                            EdgeRigidBody edgeB = *static_pointer_cast<EdgeRigidBody>(bodyB);
                            return DetectCircleEdgeCollision(circleA, edgeB, outManifold);
                        }
                        case RbShape::TerrainMesh: {
                            TerrainMeshRigidBody terrainMeshB = *static_pointer_cast<TerrainMeshRigidBody>(bodyB);
                            return DetectCircleTerrainMeshCollision(circleA, terrainMeshB, outManifold);
                        }
                        default:
                            return false;
                    }
                }
                case RbShape::Capsule: {
                    CapsuleRigidBody capsuleA = *static_pointer_cast<CapsuleRigidBody>(bodyA);
                    switch (shapeB) {
                        case RbShape::Box: {
                            BoxRigidBody boxB = *static_pointer_cast<BoxRigidBody>(bodyB);
                            return Invert(DetectBoxCapsuleCollision(boxB, capsuleA, outManifold), outManifold);
                        }
                        case RbShape::Circle: {
                            CircleRigidBody circleB = *static_pointer_cast<CircleRigidBody>(bodyB);
                            return Invert(DetectCircleCapsuleCollision(circleB, capsuleA, outManifold), outManifold);
                        }
                        case RbShape::Capsule: {
                            CapsuleRigidBody capsuleB = *static_pointer_cast<CapsuleRigidBody>(bodyB);
                            return DetectCapsuleCapsuleCollision(capsuleA, capsuleB, outManifold);
                        }
                        case RbShape::Triangle: {
                            TriangleRigidBody triangleB = *static_pointer_cast<TriangleRigidBody>(bodyB);
                            return DetectCapsuleTriangleCollision(capsuleA, triangleB, outManifold);
                        }
                        case RbShape::Edge: {
                            EdgeRigidBody edgeB = *static_pointer_cast<EdgeRigidBody>(bodyB);
                            return DetectCapsuleEdgeCollision(capsuleA, edgeB, outManifold);
                        }
                        case RbShape::TerrainMesh: {
                            TerrainMeshRigidBody terrainMeshB = *static_pointer_cast<TerrainMeshRigidBody>(bodyB);
                            return DetectCapsuleTerrainMeshCollision(capsuleA, terrainMeshB, outManifold);
                        }
                        default:
                            return false;
                    }
                }
                case RbShape::Triangle: {
                    TriangleRigidBody triangleA = *static_pointer_cast<TriangleRigidBody>(bodyA);
                    switch (shapeB) {
                        case RbShape::Box: {
                            BoxRigidBody boxB = *static_pointer_cast<BoxRigidBody>(bodyB);
                            return Invert(DetectBoxTriangleCollision(boxB, triangleA, outManifold), outManifold);
                        }
                        case RbShape::Circle: {
                            CircleRigidBody circleB = *static_pointer_cast<CircleRigidBody>(bodyB);
                            return Invert(DetectCircleTriangleCollision(circleB, triangleA, outManifold), outManifold);
                        }
                        case RbShape::Capsule: {
                            CapsuleRigidBody capsuleB = *static_pointer_cast<CapsuleRigidBody>(bodyB);
                            return Invert(DetectCapsuleTriangleCollision(capsuleB, triangleA, outManifold), outManifold);
                        }
                        case RbShape::Triangle: {
                            TriangleRigidBody triangleB = *static_pointer_cast<TriangleRigidBody>(bodyB);
                            return DetectTriangleTriangleCollision(triangleA, triangleB, outManifold);
                        }
                        case RbShape::Edge: {
                            EdgeRigidBody edgeB = *static_pointer_cast<EdgeRigidBody>(bodyB);
                            return DetectTriangleEdgeCollision(triangleA, edgeB, outManifold);
                        }
                        case RbShape::TerrainMesh: {
                            TerrainMeshRigidBody terrainMeshB = *static_pointer_cast<TerrainMeshRigidBody>(bodyB);
                            return DetectTriangleTerrainMeshCollision(triangleA, terrainMeshB, outManifold);
                        }
                        default:
                            return false;
                    }
                }
                case RbShape::Edge: {
                    EdgeRigidBody edgeA = *static_pointer_cast<EdgeRigidBody>(bodyA);
                    switch (shapeB) {
                        case RbShape::Box: {
                            BoxRigidBody boxB = *static_pointer_cast<BoxRigidBody>(bodyB);
                            return DetectBoxEdgeCollision(boxB, edgeA, outManifold);
                        }
                        case RbShape::Circle: {
                            CircleRigidBody circleB = *static_pointer_cast<CircleRigidBody>(bodyB);
                            return DetectCircleEdgeCollision(circleB, edgeA, outManifold);
                        }
                        case RbShape::Capsule: {
                            CapsuleRigidBody capsuleB = *static_pointer_cast<CapsuleRigidBody>(bodyB);
                            return DetectCapsuleEdgeCollision(capsuleB, edgeA, outManifold);
                        }
                        case RbShape::Triangle: {
                            TriangleRigidBody triangleB = *static_pointer_cast<TriangleRigidBody>(bodyB);
                            return DetectTriangleEdgeCollision(triangleB, edgeA, outManifold);
                        }
                        default:
                            return false;
                    }
                }
                case RbShape::TerrainMesh: {
                    TerrainMeshRigidBody terrainMeshA = *static_pointer_cast<TerrainMeshRigidBody>(bodyA);
                    switch (shapeB) {
                        case RbShape::Box: {
                            BoxRigidBody boxB = *static_pointer_cast<BoxRigidBody>(bodyB);
                            return DetectBoxTerrainMeshCollision(boxB, terrainMeshA, outManifold);
                        }
                        case RbShape::Circle: {
                            CircleRigidBody circleB = *static_pointer_cast<CircleRigidBody>(bodyB);
                            return DetectCircleTerrainMeshCollision(circleB, terrainMeshA, outManifold);
                        }
                        case RbShape::Capsule: {
                            CapsuleRigidBody capsuleB = *static_pointer_cast<CapsuleRigidBody>(bodyB);
                            return DetectCapsuleTerrainMeshCollision(capsuleB, terrainMeshA, outManifold);
                        }
                        case RbShape::Triangle: {
                            TriangleRigidBody triangleB = *static_pointer_cast<TriangleRigidBody>(bodyB);
                            return DetectTriangleTerrainMeshCollision(triangleB, terrainMeshA, outManifold);
                        }
                        default:
                            return false;
                    }
                }
                default:
                    return false;
            }
            return false;
        }

    private:
        static bool Invert(bool prevResult, Manifold& outManifold) {
            if (!prevResult) return false;
            for (CollisionPoint& coll : outManifold.collisionPoints) {
                coll.normal = coll.normal * -1;
            }
            std::swap(outManifold.bodyA, outManifold.bodyB);
            return true;
        }

        // box collision detection
        static bool DetectBoxBoxCollision(
             const BoxRigidBody& bodyA,
             const BoxRigidBody& bodyB,
             Manifold& outManifold
        );
        static bool DetectBoxCircleCollision(
             const BoxRigidBody& bodyA,
             const CircleRigidBody& bodyB,
             Manifold& outManifold
        );
        static bool DetectBoxCapsuleCollision(
             const BoxRigidBody& bodyA,
             const CapsuleRigidBody& bodyB,
             Manifold& outManifold
        );
        static bool DetectBoxTriangleCollision(
             const BoxRigidBody& bodyA,
             const TriangleRigidBody& bodyB,
             Manifold& outManifold
        );
        static bool DetectBoxEdgeCollision(
             const BoxRigidBody& bodyA,
             const EdgeRigidBody& bodyB,
             Manifold& outManifold
        );
        static bool DetectBoxTerrainMeshCollision(
             const BoxRigidBody& bodyA,
             const TerrainMeshRigidBody& bodyB,
             Manifold& outManifold
        );

        // circle collision detection
        static bool DetectCircleCircleCollision(
             const CircleRigidBody& bodyA,
             const CircleRigidBody& bodyB,
             Manifold& outManifold
        );
        static bool DetectCircleCapsuleCollision(
             const CircleRigidBody& bodyA,
             const CapsuleRigidBody& bodyB,
             Manifold& outManifold
        );
        static bool DetectCircleTriangleCollision(
             const CircleRigidBody& bodyA,
             const TriangleRigidBody& bodyB,
             Manifold& outManifold
        );
        static bool DetectCircleEdgeCollision(
             const CircleRigidBody& bodyA,
             const EdgeRigidBody& bodyB,
             Manifold& outManifold
        );
        static bool DetectCircleTerrainMeshCollision(
             const CircleRigidBody& bodyA,
             const TerrainMeshRigidBody& bodyB,
             Manifold& outManifold
        );

        // capsule collision detection
        static bool DetectCapsuleCapsuleCollision(
             const CapsuleRigidBody& bodyA,
             const CapsuleRigidBody& bodyB,
             Manifold& outManifold
        );
        static bool DetectCapsuleTriangleCollision(
             const CapsuleRigidBody& bodyA,
             const TriangleRigidBody& bodyB,
             Manifold& outManifold
        );
        static bool DetectCapsuleEdgeCollision(
             const CapsuleRigidBody& bodyA,
             const EdgeRigidBody& bodyB,
             Manifold& outManifold
        );
        static bool DetectCapsuleTerrainMeshCollision(
             const CapsuleRigidBody& bodyA,
             const TerrainMeshRigidBody& bodyB,
             Manifold& outManifold
        );

        // triangle collision detection
        static bool DetectTriangleTriangleCollision(
             const TriangleRigidBody& bodyA,
             const TriangleRigidBody& bodyB,
             Manifold& outManifold
        );
        static bool DetectTriangleEdgeCollision(
             const TriangleRigidBody& bodyA,
             const EdgeRigidBody& bodyB,
             Manifold& outManifold
        );
        static bool DetectTriangleTerrainMeshCollision(
             const TriangleRigidBody& bodyA,
             const TerrainMeshRigidBody& bodyB,
             Manifold& outManifold
        );

        // polygon
        static bool DetectPolygonPolygonCollision(
             const PolygonRigidBody& bodyA,
             const PolygonRigidBody& bodyB,
             Manifold& outManifold
        );
        static bool DetectPolygonCircleCollision(
             const PolygonRigidBody& bodyA,
             const CircleRigidBody& bodyB,
             Manifold& outManifold
        );
        static bool DetectPolygonCapsuleCollision(
             const PolygonRigidBody& bodyA,
             const CapsuleRigidBody& bodyB,
             Manifold& outManifold
        );
        static bool DetectPolygonEdgeCollision(
             const PolygonRigidBody& bodyA,
             const EdgeRigidBody& bodyB,
             Manifold& outManifold
        );
        static bool DetectPolygonTerrainMeshCollision(
             const PolygonRigidBody& bodyA,
             const TerrainMeshRigidBody& bodyB,
             Manifold& outManifold
        );
    };
}

#endif
