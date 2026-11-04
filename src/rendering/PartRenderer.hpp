#ifndef RENDERING_PART_RENDERER_HPP
#define RENDERING_PART_RENDERER_HPP

#include "game/Part.hpp"

class PartRenderer {
public:
    void Render(const PartRenderState& state) const;
    void RenderContained(
        const Part& part,
        const EditorGrid& grid,
        Math::GridCoord containerCoord
    ) const;
    void RenderConnectors(const Part& part, const EditorGrid& grid) const;
    void RenderGhost(
        PartType type,
        Math::GridCoord coord,
        const EditorGrid& grid,
        PartOrientation orientation
    ) const;
    void RenderGhostAtPosition(
        PartType type,
        Math::Vector2D position,
        float cellSize,
        PartOrientation orientation
    ) const;
};

#endif
