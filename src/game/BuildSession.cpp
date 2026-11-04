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
    const bool placed = m_buildModel.PlacePart(
        coord,
        m_selectedPartType.value(),
        m_selectedOrientation
    );
    return placed;
}

bool BuildSession::PickUpPart(Math::GridCoord coord) {
    std::optional<Part>& part = m_buildModel.GetGrid().GetPart(coord);
    if (!part.has_value()) return false;

    Part* partToPickUp = &part.value();
    if (partToPickUp->HasContainedPart()) {
        partToPickUp = partToPickUp->GetContainedPart();
        m_repositionedPartType = partToPickUp->GetType();
        m_repositionedOrientation = partToPickUp->GetOrientation();
        part->RemoveContainedPart();
        return true;
    }

    m_repositionedPartType = partToPickUp->GetType();
    m_repositionedOrientation = partToPickUp->GetOrientation();
    m_buildModel.GetGrid().RemovePart(coord);
    return true;
}

bool BuildSession::PlaceRepositionedPart(Math::GridCoord coord) {
    if (!m_repositionedPartType.has_value()) return false;
    const bool placed = m_buildModel.PlacePart(
        coord,
        m_repositionedPartType.value(),
        m_repositionedOrientation
    );
    if (placed) {
        m_repositionedPartType.reset();
    }
    return placed;
}

void BuildSession::DiscardRepositionedPart() {
    m_repositionedPartType.reset();
}

void BuildSession::RotatePart(Math::GridCoord coord) {
    m_buildModel.GetGrid().RotatePart(coord);
}

void BuildSession::RotateSelectedPart() {
    if (m_selectedPartType.has_value()) {
        m_selectedOrientation = RotatePartOrientation(m_selectedOrientation);
    }
}

void BuildSession::RemovePart(Math::GridCoord coord) {
    m_buildModel.RemovePart(coord);
}
