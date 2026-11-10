#ifndef PHYSISCS_COLLISION_POLYGON_HPP
#define PHYSISCS_COLLISION_POLYGON_HPP

#include <vector>
#include "math/Vector2D.hpp"
#include "math/Interval.hpp"

namespace Physics {

struct CollisionPolygon {
    std::vector<Math::Vector2D> vertices;

    static inline CollisionPolygon FromVertices(
        const std::initializer_list<Math::Vector2D>& verts
    ) {
        CollisionPolygon poly;
        poly.vertices = std::vector<Math::Vector2D>(verts);
        return poly;
    }

    inline Math::Vector2D GetCenter() const {
        Math::Vector2D center = Math::Vector2D::Zero();
        for (const auto& vertex: vertices) {
            center += vertex;
        }
        center /= vertices.size();
        return center;
    }

    // shoelace formula to calculate the signed area of the polygon
    inline Math::RealNum GetSignedArea() const {
        Math::RealNum area = 0.0;
        size_t n = vertices.size();
        for (size_t i = 0; i < n; ++i) {
            const Math::Vector2D& current = vertices[i];
            const Math::Vector2D& next = vertices[(i + 1) % n];
            area += (current.x * next.y) - (next.x * current.y);
        }
        return area / 2.0;
    }

    // get outward normal of the edge at edgeIndex
    // isCCW to assume counter clockwise ordering
    inline Math::Vector2D GetOutwardNormal(size_t edgeIndex, bool isCCW) const {
        size_t count = vertices.size();
        size_t next = (edgeIndex + 1) % count;
        Math::Vector2D edge = vertices[next] - vertices[edgeIndex];
        Math::Vector2D normal = isCCW
            ? Math::Vector2D(edge.y, -edge.x)
            : Math::Vector2D(-edge.y, edge.x);
        return normal.Normal().value_or(Math::Vector2D::Zero());
    }

    // project the polygon onto the given axis and return the interval of projections
    inline Math::Interval ProjectPolygon(const Math::Vector2D& axis) const {
        Math::RealNum minProjection = Math::RealNum::Infinity();
        Math::RealNum maxProjection = -Math::RealNum::Infinity();
        for (const auto& vertex: vertices) {
            Math::RealNum projection = vertex.Dot(axis);
            minProjection = std::min(minProjection, projection);
            maxProjection = std::max(maxProjection, projection);
        }
        return Math::Interval(minProjection, maxProjection);
    }

    // test the axes of the source polygon against the other polygon
    inline bool TestAxes(
        const CollisionPolygon& source,
        const CollisionPolygon& other,
        const CollisionPolygon& pA,
        const CollisionPolygon& pB,
        bool isCCW,
        bool sourceIsA,
        Math::RealNum& minDepth,
        int& bestEdgeIndex,
        bool& bestEdgeFromA
    ) const {
        size_t count = source.vertices.size();
        for (size_t i = 0; i < count; ++i) {
            Math::Vector2D axis = source.GetOutwardNormal(i, isCCW);
            if (!axis.TryNormalize())
                continue;

            Math::Interval intervalA = pA.ProjectPolygon(axis);
            Math::Interval intervalB = pB.ProjectPolygon(axis);

            auto overlapOpt = intervalA.GetOverlap(intervalB);
            if (!overlapOpt.has_value())
                return false;
            Math::RealNum overlap = overlapOpt.value();

            if (overlap < minDepth) {
                minDepth = overlap;
                bestEdgeIndex = static_cast<int>(i);
                bestEdgeFromA = sourceIsA;
            }
        }
        return true;
    }

    // find incident edge on an arbitrary polygon
    int FindReferenceEdge(
        const Math::Vector2D& referenceNormal,
        bool isCCW
    ) const {
        int bestEdge = -1;
        size_t count = vertices.size();
        Math::RealNum maxDot = - Math::RealNum::Infinity();

        for (size_t i = 0; i < count; ++i) {
            Math::Vector2D normal = GetOutwardNormal(i, isCCW);
            Math::RealNum dot = normal.Dot(referenceNormal);
            if (dot > maxDot) {
                maxDot = dot;
                bestEdge = static_cast<int>(i);
            }
        }
        return bestEdge;
    }

    // find incident edge on an arbitrary polygon
    int FindIncidentEdge(
        const Math::Vector2D& referenceNormal,
        bool isCCW
    ) const {
        int bestEdge = -1;
        size_t count = vertices.size();
        Math::RealNum minDot = Math::RealNum::Infinity();

        for (size_t i = 0; i < count; ++i) {
            Math::Vector2D normal = GetOutwardNormal(i, isCCW);
            Math::RealNum dot = normal.Dot(referenceNormal);
            if (dot < minDot) {
                minDot = dot;
                bestEdge = static_cast<int>(i);
            }
        }
        return bestEdge;
    }

    inline CollisionPolygon ClipSegmentToLine(
        const Math::Vector2D& normal,
        Math::RealNum offset
    ) {
        if (vertices.size() < 2) return *this;

        CollisionPolygon clipped;
        Math::RealNum distanceA = vertices[0].Dot(normal) - offset;
        Math::RealNum distanceB = vertices[1].Dot(normal) - offset;

        if (distanceA <= Math::RealNum::epsilon)
            clipped.vertices.push_back(vertices[0]);
        if (distanceB <= Math::RealNum::epsilon)
            clipped.vertices.push_back(vertices[1]);

        if (distanceA * distanceB < 0.0f) {
            Math::RealNum t = distanceA / (distanceA - distanceB);
            clipped.vertices.push_back(
                vertices[0] + (vertices[1] - vertices[0]) * t
            );
        }
        return clipped;
    }
};

}

#endif
