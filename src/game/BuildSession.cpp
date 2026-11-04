#include "game/BuildSession.hpp"

void BuildSession::SelectPartType(PartType type) {
    if (m_selectedPartType.has_value() && m_selectedPartType.value() == type) {
        m_selectedPartType.reset();
        return;
    }

    m_selectedPartType = type;
    m_selectedOrientation = PartOrientation::Up;
}

bool BuildSession::PlaceSelectedPart(Math::GridCoord coord) {
    if (!m_selectedPartType.has_value()) return false;
    return m_buildModel.PlacePart(
        coord,
        m_selectedPartType.value(),
        m_selectedOrientation
    );
}

void BuildSession::RotateSelectedPart() {
    if (m_selectedPartType.has_value()) {
        m_selectedOrientation = RotatePartOrientation(m_selectedOrientation);
    }
}

void BuildSession::RemovePart(Math::GridCoord coord) {
    m_buildModel.RemovePart(coord);
}
