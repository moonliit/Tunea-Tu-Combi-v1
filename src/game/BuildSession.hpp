#ifndef GAME_BUILD_SESSION_HPP
#define GAME_BUILD_SESSION_HPP

#include "game/BuildModel.hpp"

#include <optional>

class BuildSession {
private:
    BuildModel& m_buildModel;
    std::optional<PartType> m_selectedPartType;
    PartOrientation m_selectedOrientation = PartOrientation::Up;

public:
    explicit BuildSession(BuildModel& buildModel): m_buildModel(buildModel) {}

    EditorGrid& GetGrid() { return m_buildModel.GetGrid(); }
    const EditorGrid& GetGrid() const { return m_buildModel.GetGrid(); }
    BuildModel& GetBuildModel() { return m_buildModel; }
    const BuildModel& GetBuildModel() const { return m_buildModel; }

    void SelectPartType(PartType type);
    void ClearSelection() { m_selectedPartType.reset(); }
    const std::optional<PartType>& GetSelectedPartType() const {
        return m_selectedPartType;
    }
    PartOrientation GetSelectedOrientation() const { return m_selectedOrientation; }

    bool PlaceSelectedPart(Math::GridCoord coord);
    void RotateSelectedPart();
    void RemovePart(Math::GridCoord coord);
};

#endif
