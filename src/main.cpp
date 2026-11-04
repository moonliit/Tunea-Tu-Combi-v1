#include "game/Game.hpp"

namespace {

TerrainDefinition CreateTerrainDefinition(bool useComplexTerrain) {
    if (!useComplexTerrain) {
        return TerrainDefinition{
            .polygons = {
                TerrainPolygon{
                    .segments = {
                        TerrainSegment{
                            .type = TerrainSegmentType::Line,
                            .points = {
                                {100.0f, 450.0f},
                                {300.0f, 450.0f}
                            }
                        },
                        TerrainSegment{
                            .type = TerrainSegmentType::CatmullRom,
                            .points = {
                                {300.0f, 450.0f},
                                {300.0f, 650.0f},
                                {100.0f, 650.0f},
                            }
                        },
                        TerrainSegment{
                            .type = TerrainSegmentType::Line,
                            .points = {
                                {100.0f, 650.0f},
                                {100.0f, 550.0f},
                                {100.0f, 450.0f}
                            }
                        }
                    },
                }
            }
        };
    }

    return TerrainDefinition{
        .polygons = {
            TerrainPolygon{
                .segments = {
                    TerrainSegment{
                        .type = TerrainSegmentType::Line,
                        .points = {{-100.0f, 500.0f}, {80.0f, 500.0f}}
                    },
                    TerrainSegment{
                        .type = TerrainSegmentType::CatmullRom,
                        .points = {
                            {80.0f, 500.0f},
                            {150.0f, 470.0f},
                            {220.0f, 430.0f},
                            {290.0f, 470.0f},
                            {350.0f, 410.0f}
                        },
                        .resolution = 12
                    },
                    TerrainSegment{
                        .type = TerrainSegmentType::Line,
                        .points = {{350.0f, 410.0f}, {430.0f, 410.0f}}
                    },
                    TerrainSegment{
                        .type = TerrainSegmentType::CatmullRom,
                        .points = {
                            {430.0f, 410.0f},
                            {490.0f, 450.0f},
                            {550.0f, 520.0f},
                            {620.0f, 450.0f},
                            {680.0f, 470.0f}
                        },
                        .resolution = 12
                    },
                    TerrainSegment{
                        .type = TerrainSegmentType::Line,
                        .points = {
                            {680.0f, 470.0f},
                            {780.0f, 470.0f},
                            {780.0f, 700.0f},
                            {-100.0f, 700.0f},
                            {-100.0f, 500.0f}
                        }
                    }
                },
            },
            TerrainPolygon{
                .segments = {
                    TerrainSegment{
                        .type = TerrainSegmentType::Line,
                        .points = {
                            {820.0f, 470.0f},
                            {980.0f, 470.0f},
                            {980.0f, 700.0f},
                            {820.0f, 700.0f},
                            {820.0f, 470.0f}
                        }
                    }
                },
            }
        }
    };
}

}

int main() {
    const int windowWidth = 800;
    const int windowHeight = 600;
    const std::string windowTitle = "Bad Piggies Clone - Phase 2";

    const LevelSpecs specs = {
        .terrain = CreateTerrainDefinition(true),
        .grid = GridDefinition{
            .dims = {6, 4},
            .worldPos = {300.0f, 395.0f},
            .cellSize = 40.0f
        },
        .initialCamera = CameraDefinition{
            .target = {300.0f, 360.0f},
            .zoom = 0.72f
        },
        .goal = GoalDefinition{
            .position = {900.0f, 430.0f}
        }
    };

    Game game(windowWidth, windowHeight, windowTitle, specs);
    game.Run();
    return 0;
}
