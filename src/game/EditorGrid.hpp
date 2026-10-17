#ifndef EDITOR_GRID_HPP
#define EDITOR_GRID_HPP

#include "game/Part.hpp"
#include <optional>
#include <vector>

class EditorGrid {
private:
    int m_width;
    int m_height;
    std::vector<std::optional<Part>> m_cells;
    std::vector<Part*> m_activeParts;
    float m_cellSize;

    size_t GetIndex(int x, int y) const {
        return y * m_width + x;
    }

public:
    EditorGrid(int width, int height, float cellSize) : m_width(width), m_height(height), m_cellSize(cellSize) {
        m_cells.resize(m_width * m_height, std::nullopt);
    }

    bool IsWithinBounds(int x, int y) const {
        return x >= 0 && x < m_width && y >= 0 && y < m_height;
    }

    bool PlacePart(int x, int y, PartType type) {
        if (!IsWithinBounds(x, y)) return false;
        size_t idx = GetIndex(x, y);

        if (m_cells[idx].has_value()) {
            RemovePart(x, y);
        }

        m_cells[idx] = Part({x, y}, type);
        m_activeParts.push_back(&m_cells[idx].value());
        m_cells[idx].value().m_activeCacheIndex = m_activeParts.size() - 1;
        return true;
    }

    void RemovePart(int x, int y) {
        if (!IsWithinBounds(x, y)) return;
        size_t idx = GetIndex(x, y);

        if (!m_cells[idx].has_value()) return;

        Part& partToRemove = m_cells[idx].value();
        size_t cacheIdx = partToRemove.m_activeCacheIndex;

        if (cacheIdx < m_activeParts.size() - 1) {
            m_activeParts[cacheIdx] = m_activeParts.back();
            m_activeParts[cacheIdx]->m_activeCacheIndex = cacheIdx;
        }
        m_activeParts.pop_back();
        m_cells[idx].reset();
    }

    const std::optional<Part>& GetPart(int x, int y) const {
        size_t idx = GetIndex(x, y);
        return m_cells[idx];
    }

    const std::vector<Part*>& GetActiveParts() const {
        return m_activeParts;
    }

    float GetCellSize() const { return m_cellSize; }

    void Clear() {
        m_cells.assign(m_width * m_height, std::nullopt);
        m_activeParts.clear();
    }
};

#endif
