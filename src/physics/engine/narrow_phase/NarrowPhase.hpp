#ifndef PHYSICS_NARROW_PHASE_HPP
#define PHYSICS_NARROW_PHASE_HPP

#include <memory>
#include "physics/engine/Common.hpp"
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
                    BoxRigidBody boxA = static_cast<const BoxRigidBody&>(*bodyA);
                    switch (shapeB) {
                        case RbShape::Box: {
                            BoxRigidBody boxB = static_cast<const BoxRigidBody&>(*bodyB);
                            return DetectBoxBoxCollision(boxA, boxB, outManifold);
                        }
                        case RbShape::Circle: {
                            CircleRigidBody circleB = static_cast<const CircleRigidBody&>(*bodyB);
                            return DetectBoxCircleCollision(boxA, circleB, outManifold);
                        }
                        case RbShape::Capsule: {
                            CapsuleRigidBody capsuleB = static_cast<const CapsuleRigidBody&>(*bodyB);
                            return DetectBoxCapsuleCollision(boxA, capsuleB, outManifold);
                        }
                        case RbShape::Triangle: {
                            TriangleRigidBody triangleB = static_cast<const TriangleRigidBody&>(*bodyB);
                            return DetectBoxTriangleCollision(boxA, triangleB, outManifold);
                        }
                        case RbShape::Edge: {
                            EdgeRigidBody edgeB = static_cast<const EdgeRigidBody&>(*bodyB);
                            return DetectBoxEdgeCollision(boxA, edgeB, outManifold);
                        }
                        default:
                            return false;
                    }
                }
                case RbShape::Circle: {
                    CircleRigidBody circleA = static_cast<const CircleRigidBody&>(*bodyA);
                    switch (shapeB) {
                        case RbShape::Box: {
                            BoxRigidBody boxB = static_cast<const BoxRigidBody&>(*bodyB);
                            return DetectBoxCircleCollision(boxB, circleA, outManifold);
                        }
                        case RbShape::Circle: {
                            CircleRigidBody circleB = static_cast<const CircleRigidBody&>(*bodyB);
                            return DetectCircleCircleCollision(circleA, circleB, outManifold);
                        }
                        case RbShape::Capsule: {
                            CapsuleRigidBody capsuleB = static_cast<const CapsuleRigidBody&>(*bodyB);
                            return DetectCircleCapsuleCollision(circleA, capsuleB, outManifold);
                        }
                        case RbShape::Triangle: {
                            TriangleRigidBody triangleB = static_cast<const TriangleRigidBody&>(*bodyB);
                            return DetectCircleTriangleCollision(circleA, triangleB, outManifold);
                        }
                        case RbShape::Edge: {
                            EdgeRigidBody edgeB = static_cast<const EdgeRigidBody&>(*bodyB);
                            return DetectCircleEdgeCollision(circleA, edgeB, outManifold);
                        }
                        default:
                            return false;
                    }
                }
                case RbShape::Capsule: {
                    CapsuleRigidBody capsuleA = static_cast<const CapsuleRigidBody&>(*bodyA);
                    switch (shapeB) {
                        case RbShape::Box: {
                            BoxRigidBody boxB = static_cast<const BoxRigidBody&>(*bodyB);
                            return DetectBoxCapsuleCollision(boxB, capsuleA, outManifold);
                        }
                        case RbShape::Circle: {
                            CircleRigidBody circleB = static_cast<const CircleRigidBody&>(*bodyB);
                            return DetectCircleCapsuleCollision(circleB, capsuleA, outManifold); // or dedicated function
                        }
                        case RbShape::Capsule: {
                            CapsuleRigidBody capsuleB = static_cast<const CapsuleRigidBody&>(*bodyB);
                            return DetectCapsuleCapsuleCollision(capsuleA, capsuleB, outManifold);
                        }
                        case RbShape::Triangle: {
                            TriangleRigidBody triangleB = static_cast<const TriangleRigidBody&>(*bodyB);
                            return DetectCapsuleTriangleCollision(capsuleA, triangleB, outManifold);
                        }
                        case RbShape::Edge: {
                            EdgeRigidBody edgeB = static_cast<const EdgeRigidBody&>(*bodyB);
                            return DetectCapsuleEdgeCollision(capsuleA, edgeB, outManifold);
                        }
                        default:
                            return false;
                    }
                }
                case RbShape::Triangle: {
                    TriangleRigidBody triangleA = static_cast<const TriangleRigidBody&>(*bodyA);
                    switch (shapeB) {
                        case RbShape::Box: {
                            BoxRigidBody boxB = static_cast<const BoxRigidBody&>(*bodyB);
                            return DetectBoxTriangleCollision(boxB, triangleA, outManifold);
                        }
                        case RbShape::Circle: {
                            CircleRigidBody circleB = static_cast<const CircleRigidBody&>(*bodyB);
                            return DetectCircleTriangleCollision(circleB, triangleA, outManifold);
                        }
                        case RbShape::Capsule: {
                            CapsuleRigidBody capsuleB = static_cast<const CapsuleRigidBody&>(*bodyB);
                            return DetectCapsuleTriangleCollision(capsuleB, triangleA, outManifold);
                        }
                        case RbShape::Triangle: {
                            TriangleRigidBody triangleB = static_cast<const TriangleRigidBody&>(*bodyB);
                            return DetectTriangleTriangleCollision(triangleA, triangleB, outManifold);
                        }
                        case RbShape::Edge: {
                            EdgeRigidBody edgeB = static_cast<const EdgeRigidBody&>(*bodyB);
                            return DetectTriangleEdgeCollision(triangleA, edgeB, outManifold);
                        }
                        default:
                            return false;
                    }
                }
                case RbShape::Edge: {
                    EdgeRigidBody edgeA = static_cast<const EdgeRigidBody&>(*bodyA);
                    switch (shapeB) {
                        case RbShape::Box: {
                            BoxRigidBody boxB = static_cast<const BoxRigidBody&>(*bodyB);
                            return DetectBoxEdgeCollision(boxB, edgeA, outManifold);
                        }
                        case RbShape::Circle: {
                            CircleRigidBody circleB = static_cast<const CircleRigidBody&>(*bodyB);
                            return DetectCircleEdgeCollision(circleB, edgeA, outManifold);
                        }
                        case RbShape::Capsule: {
                            CapsuleRigidBody capsuleB = static_cast<const CapsuleRigidBody&>(*bodyB);
                            return DetectCapsuleEdgeCollision(capsuleB, edgeA, outManifold);
                        }
                        case RbShape::Triangle: {
                            TriangleRigidBody triangleB = static_cast<const TriangleRigidBody&>(*bodyB);
                            return DetectTriangleEdgeCollision(triangleB, edgeA, outManifold);
                        }
                        case RbShape::Edge: {
                            return false;
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
    };
}

#endif
