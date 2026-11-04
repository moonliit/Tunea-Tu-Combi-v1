#ifndef RENDERING_PART_RENDERER_HPP
#define RENDERING_PART_RENDERER_HPP

#include "game/Part.hpp"

class PartRenderer {
public:
    void Render(const PartRenderState& state) const;
    void RenderConnectors(const Part& part, const EditorGrid& grid) const;
    void RenderGhost(
        PartType type,
        Math::GridCoord coord,
        const EditorGrid& grid,
        PartOrientation orientation
    ) const;
};

#endif
