#ifndef BUILD_SESSION_HPP
#define BUILD_SESSION_HPP

#include "app/game/BuildModel.hpp"
#include <optional>

class BuildSession {
private:
    BuildModel& m_buildModel;
    std::optional<PartType> m_selectedPartType;
    PartOrientation m_selectedOrientation = PartOrientation::Up;
    bool m_selectedIsDefaultOrientation = true;
    std::optional<PartType> m_repositionedPartType;
    PartOrientation m_repositionedOrientation = PartOrientation::Up;

public:
    explicit BuildSession(BuildModel& buildModel): m_buildModel(buildModel) {}

    EditorGrid& GetGrid() { return m_buildModel.GetGrid(); }
    const EditorGrid& GetGrid() const { return m_buildModel.GetGrid(); }
    BuildModel& GetBuildModel() { return m_buildModel; }
    const BuildModel& GetBuildModel() const { return m_buildModel; }

    void SelectPartType(PartType type);
    void ClearSelection() { 
        m_selectedPartType.reset(); 
        m_selectedIsDefaultOrientation = true; 
    }
    void ClearHand() {
        m_selectedPartType.reset();
        m_selectedIsDefaultOrientation = true;
        m_repositionedPartType.reset();
    }
    bool HasHandItem() const {
        return m_selectedPartType.has_value() || m_repositionedPartType.has_value();
    }
    const std::optional<PartType>& GetSelectedPartType() const {
        return m_selectedPartType;
    }
    PartOrientation GetSelectedOrientation() const { return m_selectedOrientation; }
    bool GetSelectedIsDefaultOrientation() const {
        return m_selectedIsDefaultOrientation;
    }
    bool HasRepositionedPart() const { return m_repositionedPartType.has_value(); }
    const std::optional<PartType>& GetRepositionedPartType() const {
        return m_repositionedPartType;
    }
    PartOrientation GetRepositionedOrientation() const {
        return m_repositionedOrientation;
    }

    bool PlaceSelectedPart(Math::GridCoord coord);
    bool PickUpPart(Math::GridCoord coord);
    bool PlaceRepositionedPart(Math::GridCoord coord);
    void DiscardRepositionedPart();
    void RotatePart(Math::GridCoord coord);
    void RotateSelectedPart();
    void RemovePart(Math::GridCoord coord);
};

#endif
