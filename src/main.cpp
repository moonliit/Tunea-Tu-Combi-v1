#include "app/App.hpp"
using namespace Math;

namespace {

TerrainDefinition CreateTerrainDefinition(bool useComplexTerrain) {
    if (!useComplexTerrain) {

return TerrainDefinition{
    .polygons = {
        TerrainPolygon{
            .segments = {
                TerrainSegment{
                    .type = TerrainSegmentType::CatmullRom,
                    .points = {
                        {-370.0f, 22.0f},
                        {-320.0f, -45.0f},
                        {-263.0f, -70.0f},
                        {-219.0f, -78.0f},
                        {-172.0f, -69.0f},
                        {-123.0f, -50.0f},
                        {-69.0f, -46.0f},
                        {1.0f, -83.0f},
                        {-9.0f, -145.0f},
                        {-60.0f, -200.0f},
                        {-122.0f, -208.0f},
                        {-149.0f, -234.0f},
                        {-119.0f, -267.0f},
                        {-27.0f, -276.0f},
                        {34.0f, -221.0f},
                        {64.0f, -127.0f},
                        {66.0f, -70.0f},
                        {56.0f, -23.0f},
                        {29.0f, 25.0f},
                        {-22.0f, 83.0f},
                        {-147.0f, 133.0f},
                    }
                },
                TerrainSegment{
                    .type = TerrainSegmentType::Line,
                    .points = {
                        {-147.0f, 133.0f},
                        {-323.0f, 179.0f},
                        {-169.0f, 24.0f},
                        {-265.4f, 90.7f},
                        {-468.4f, 112.7f},
                        {-612.5f, 84.7f},
                    }
                },
                TerrainSegment{
                    .type = TerrainSegmentType::CatmullRom,
                    .points = {
                        {-612.5f, 84.7f},
                        {-719.5f, 110.7f},
                        {-634.5f, 10.7f},
                        {-684.5f, -77.3f},
                        {-556.5f, -11.3f},
                        {-464.5f, 4.7f},
                        {-419.5f, 10.7f},
                    }
                },
                TerrainSegment{
                    .type = TerrainSegmentType::Line,
                    .points = {
                        {-419.5f, 10.7f},
                        {-370.0f, 22.0f},
                    }
                },
            },
        },
        TerrainPolygon{
            .segments = {
                TerrainSegment{
                    .type = TerrainSegmentType::Line,
                    .points = {
                        {-300.5f, -278.5f},
                        {-277.7f, -174.2f},
                        {-443.4f, -204.2f},
                        {-351.9f, -225.7f},
                        {-631.9f, -408.5f},
                        {-577.7f, -449.9f},
                        {-323.4f, -252.8f},
                        {-314.8f, -341.4f},
                        {-300.5f, -278.5f},
                    }
                },
            },
        },
        TerrainPolygon{
            .segments = {
                TerrainSegment{
                    .type = TerrainSegmentType::CatmullRom,
                    .points = {
                        {-855.3f, -55.2f},
                        {-796.8f, -153.8f},
                        {-688.2f, -179.5f},
                        {-593.9f, -149.5f},
                        {-673.9f, -118.0f},
                    }
                },
                TerrainSegment{
                    .type = TerrainSegmentType::Line,
                    .points = {
                        {-673.9f, -118.0f},
                        {-511.0f, -130.9f},
                        {-576.8f, -289.5f},
                        {-563.9f, -189.5f},
                        {-681.0f, -263.8f},
                    }
                },
                TerrainSegment{
                    .type = TerrainSegmentType::CatmullRom,
                    .points = {
                        {-681.0f, -263.8f},
                        {-789.6f, -236.6f},
                        {-859.6f, -175.2f},
                        {-895.3f, -86.6f},
                        {-898.2f, -52.3f},
                        {-855.3f, -55.2f},
                    }
                },
            },
        },
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
        .terrain = TerrainDefinition{
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
        },
        .grid = GridDefinition{
            .dims = {6, 4},
            .worldPos = {-50.0f, 480.0f},
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

    App app(windowWidth, windowHeight, windowTitle, specs);
    app.Run();
    return 0;
}
