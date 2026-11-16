#include "math/Terrain.hpp"
#include "math/Spline.hpp"
#include "physics/engine/World.hpp"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <memory>
#include <numeric>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

using Clock = std::chrono::steady_clock;
constexpr int WarmupSteps = 180;
constexpr int MeasuredSteps = 240;
constexpr int Trials = 9;
constexpr float StepSeconds = 1.0f / 60.0f;

Math::TerrainDefinition ReadTerrain() {
    const std::string path = std::string(TERRAIN_PROOF_SOURCE_DIR) +
        "/assets/levels/default.json";
    std::ifstream file(path);
    if (!file) throw std::runtime_error("No se pudo leer el nivel base");
    const auto data = nlohmann::json::parse(file);
    Math::TerrainDefinition terrain;
    for (const auto& polygonData : data.at("terrain").at("polygons")) {
        Math::TerrainPolygon polygon;
        for (const auto& segmentData : polygonData.at("segments")) {
            Math::TerrainSegment segment;
            segment.type = segmentData.at("type") == "CatmullRom"
                ? Math::TerrainSegmentType::CatmullRom
                : Math::TerrainSegmentType::Line;
            segment.resolution = segmentData.value("resolution", 8);
            for (const auto& point : segmentData.at("points")) {
                segment.points.push_back({
                    point.at("x").get<float>(), point.at("y").get<float>()});
            }
            polygon.segments.push_back(std::move(segment));
        }
        terrain.polygons.push_back(std::move(polygon));
    }
    return terrain;
}

std::vector<Math::Vector2D> SampleBoundary(
    const Math::TerrainDefinition& terrain, bool adaptive) {
    const auto& segments = terrain.polygons.at(0).segments;
    if (segments.size() < 3 ||
        segments[1].type != Math::TerrainSegmentType::CatmullRom) {
        throw std::runtime_error("El nivel base no tiene la curva esperada");
    }
    std::vector<Math::Vector2D> boundary;
    for (const auto& segment : segments) {
        std::vector<Math::Vector2D> sampled;
        if (segment.type == Math::TerrainSegmentType::CatmullRom) {
            sampled = adaptive
                ? Math::SampleCatmullRomAdaptive(
                    segment.points, Math::TerrainCurveTolerance)
                : Math::SampleCatmullRom(segment.points,
                    std::max(1, segment.resolution));
        } else {
            sampled = segment.points;
        }
        if (!boundary.empty() && !sampled.empty() &&
            boundary.back() == sampled.front()) sampled.erase(sampled.begin());
        boundary.insert(boundary.end(), sampled.begin(), sampled.end());
    }
    return boundary;
}

struct TrialResult {
    double milliseconds = 0.0;
    std::vector<Math::Vector2D> wheelPositions;
    double maxSurfaceGap = 0.0;
};

TrialResult RunTrial(const std::vector<Math::Vector2D>& boundary) {
    Physics::World world({0.0f, 392.0f}, 10, 8, 3);
    if (!world.CreateTerrainPolygon(boundary)) {
        throw std::runtime_error("No se pudo crear la colisión del terreno");
    }
    if (world.GetBodies().size() !=
        Math::BuildTerrainPolygonMesh(boundary).indices.size()/3) {
        throw std::runtime_error("La malla y los cuerpos de colisión difieren");
    }
    std::vector<std::shared_ptr<I_RigidBody>> wheels;
    for (float x : {-700.0f, -600.0f, -500.0f, -400.0f,
                    400.0f, 500.0f, 600.0f, 700.0f}) {
        const float surfaceY = x < 0.0f
            ? float(boundary.front().y)
            : float(boundary[boundary.size()-4].y);
        wheels.push_back(world.CreateCircle(
            {x, surfaceY-60.0f}, 10.0f, 1.0f, BodyType::Dynamic));
    }
    for (int step = 0; step < WarmupSteps; ++step) world.Step(StepSeconds);
    const auto start = Clock::now();
    for (int step = 0; step < MeasuredSteps; ++step) world.Step(StepSeconds);
    const auto end = Clock::now();
    std::vector<Math::Vector2D> positions;
    double maxSurfaceGap = 0.0;
    for (const auto& wheel : wheels) {
        positions.push_back(wheel->GetPosition());
    }
    for (size_t i = 0; i < positions.size(); ++i) {
        const float surfaceY = i < 4
            ? float(boundary.front().y)
            : float(boundary[boundary.size()-4].y);
        maxSurfaceGap = std::max(maxSurfaceGap,
            std::abs(double(positions[i].y) - (surfaceY-10.0)));
    }
    return {
        std::chrono::duration<double, std::milli>(end-start).count(),
        std::move(positions),
        maxSurfaceGap
    };
}

float DistanceToPolyline(const Math::Vector2D& point,
                         const std::vector<Math::Vector2D>& polyline) {
    float nearest = INFINITY;
    for (size_t i = 1; i < polyline.size(); ++i) {
        const float ax = float(polyline[i-1].x);
        const float ay = float(polyline[i-1].y);
        const float dx = float(polyline[i].x)-ax;
        const float dy = float(polyline[i].y)-ay;
        const float length = dx*dx+dy*dy;
        const float t = length > 0.0f
            ? std::clamp(((float(point.x)-ax)*dx+
                (float(point.y)-ay)*dy)/length, 0.0f, 1.0f)
            : 0.0f;
        nearest = std::min(nearest,
            std::hypot(float(point.x)-ax-t*dx, float(point.y)-ay-t*dy));
    }
    return nearest;
}

float CurveError(const Math::TerrainSegment& segment, bool adaptive) {
    const auto& points = segment.points;
    const auto sampled = adaptive
        ? Math::SampleCatmullRomAdaptive(points, Math::TerrainCurveTolerance)
        : Math::SampleCatmullRom(points, std::max(1, segment.resolution));
    float maximum = 0.0f;
    for (size_t i = 0; i+1 < points.size(); ++i) {
        const auto p0 = points[i == 0 ? i : i-1];
        const auto p1 = points[i];
        const auto p2 = points[i+1];
        const auto p3 = points[i+2 < points.size() ? i+2 : i+1];
        for (int step = 1; step < 256; ++step) {
            const auto reference = Math::EvaluateCatmullRom(
                p0,p1,p2,p3,float(step)/256.0f);
            maximum = std::max(maximum,
                DistanceToPolyline(reference, sampled));
        }
    }
    return maximum;
}

double Median(std::vector<double> values) {
    std::sort(values.begin(), values.end());
    return values[values.size()/2];
}

struct TraversalResult {
    double milliseconds = 0.0;
    Math::Vector2D finish{0.0f, 0.0f};
};

TraversalResult TraverseTerrain(const std::vector<Math::Vector2D>& boundary) {
    Physics::World world({0.0f, 392.0f}, 10, 8, 3);
    if (!world.CreateTerrainPolygon(boundary)) {
        throw std::runtime_error("No se pudo crear el terreno de recorrido");
    }
    if (world.GetBodies().size() !=
        Math::BuildTerrainPolygonMesh(boundary).indices.size()/3) {
        throw std::runtime_error("La malla y los cuerpos de colisión difieren");
    }
    auto wheel = world.CreateCircle(
        {-700.0f, float(boundary.front().y)-10.0f},
        10.0f, 1.0f, BodyType::Dynamic);
    wheel->ApplyLinearImpulse({800.0f, 0.0f});
    const auto start = Clock::now();
    for (int step = 0; step < 600; ++step) world.Step(StepSeconds);
    const auto end = Clock::now();
    return {
        std::chrono::duration<double, std::milli>(end-start).count(),
        wheel->GetPosition()
    };
}

}

int main() {
    try {
        const auto terrain = ReadTerrain();
        const auto& segments = terrain.polygons.at(0).segments;
        const auto& start = segments.front().points;
        const auto& finish = segments.back().points;
        if (start.size() < 2 || finish.size() < 2 ||
            std::abs(float(start[0].y-start[1].y)) > 0.01f ||
            std::abs(float(finish[0].y-finish[1].y)) > 0.01f ||
            float(start[0].x) > -700.0f || float(start[1].x) < -400.0f ||
            float(finish[0].x) > 400.0f || float(finish[1].x) < 700.0f) {
            throw std::runtime_error(
                "El nivel base debe comenzar y terminar con tramos planos para la prueba");
        }
        const auto fixed = SampleBoundary(terrain, false);
        const auto adaptive = SampleBoundary(terrain, true);
        const auto fixedMesh = Math::BuildTerrainPolygonMesh(fixed);
        const auto adaptiveMesh = Math::BuildTerrainPolygonMesh(adaptive);
        if (fixedMesh.indices.empty() || adaptiveMesh.indices.empty()) {
            throw std::runtime_error("Una de las mallas es inválida");
        }
        std::vector<double> fixedTimes, adaptiveTimes;
        double finalDifference = 0.0;
        double maxSurfaceGap = 0.0;
        for (int trial = 0; trial < Trials; ++trial) {
            TrialResult fixedResult, adaptiveResult;
            if (trial % 2 == 0) {
                fixedResult = RunTrial(fixed);
                adaptiveResult = RunTrial(adaptive);
            } else {
                adaptiveResult = RunTrial(adaptive);
                fixedResult = RunTrial(fixed);
            }
            fixedTimes.push_back(fixedResult.milliseconds);
            adaptiveTimes.push_back(adaptiveResult.milliseconds);
            maxSurfaceGap = std::max({maxSurfaceGap,
                fixedResult.maxSurfaceGap, adaptiveResult.maxSurfaceGap});
            for (size_t wheel = 0; wheel < fixedResult.wheelPositions.size(); ++wheel) {
                const auto& a = fixedResult.wheelPositions[wheel];
                const auto& b = adaptiveResult.wheelPositions[wheel];
                finalDifference = std::max(finalDifference,
                    std::hypot(double(a.x)-double(b.x),
                               double(a.y)-double(b.y)));
            }
        }
        const double fixedMedian = Median(fixedTimes);
        const double adaptiveMedian = Median(adaptiveTimes);
        std::vector<double> fixedTraversalTimes, adaptiveTraversalTimes;
        Math::Vector2D fixedFinish, adaptiveFinish;
        for (int trial = 0; trial < Trials; ++trial) {
            TraversalResult fixedResult, adaptiveResult;
            if (trial % 2 == 0) {
                fixedResult = TraverseTerrain(fixed);
                adaptiveResult = TraverseTerrain(adaptive);
            } else {
                adaptiveResult = TraverseTerrain(adaptive);
                fixedResult = TraverseTerrain(fixed);
            }
            fixedTraversalTimes.push_back(fixedResult.milliseconds);
            adaptiveTraversalTimes.push_back(adaptiveResult.milliseconds);
            fixedFinish = fixedResult.finish;
            adaptiveFinish = adaptiveResult.finish;
        }
        const float flatY = float(finish[0].y);
        if (float(fixedFinish.x) < 400.0f || float(fixedFinish.x) > 1100.0f ||
            float(adaptiveFinish.x) < 400.0f ||
            float(adaptiveFinish.x) > 1100.0f ||
            std::abs(float(fixedFinish.y) - (flatY-10.0f)) > 0.5f ||
            std::abs(float(adaptiveFinish.y) - (flatY-10.0f)) > 0.5f) {
            throw std::runtime_error("La rueda no terminó sobre el tramo plano");
        }
        std::cout << std::fixed << std::setprecision(3);
        std::cout << "nivel=default, warmup=" << WarmupSteps
            << ", pasos_medidos=" << MeasuredSteps
            << ", repeticiones=" << Trials << "\n";
        std::cout << "fijo: segmentos=" << fixed.size()-1
            << ", triangulos=" << fixedMesh.indices.size()/3
            << ", mediana_ms=" << fixedMedian
            << ", ms_por_paso=" << fixedMedian/MeasuredSteps << "\n";
        std::cout << "adaptativo: segmentos=" << adaptive.size()-1
            << ", triangulos=" << adaptiveMesh.indices.size()/3
            << ", mediana_ms=" << adaptiveMedian
            << ", ms_por_paso=" << adaptiveMedian/MeasuredSteps << "\n";
        for (const auto& polygon : terrain.polygons) {
            for (const auto& segment : polygon.segments) {
                if (segment.type != Math::TerrainSegmentType::CatmullRom) continue;
                std::cout << "error_curva_fijo_u=" << CurveError(segment,false)
                    << ", error_curva_adaptativo_u="
                    << CurveError(segment,true) << "\n";
            }
        }
        std::cout << "cambio_porcentual="
            << 100.0*(adaptiveMedian/fixedMedian-1.0)
            << ", maxima_diferencia_posicion_rueda_u="
            << finalDifference
            << ", max_distancia_rueda_superficie_u=" << maxSurfaceGap << "\n";
        std::cout << "recorrido_fijo_x=" << float(fixedFinish.x)
            << ", recorrido_fijo_y=" << float(fixedFinish.y)
            << ", recorrido_adaptativo_x=" << float(adaptiveFinish.x)
            << ", recorrido_adaptativo_y=" << float(adaptiveFinish.y)
            << ", diferencia_posicion_recorrido_u="
            << std::hypot(double(fixedFinish.x)-double(adaptiveFinish.x),
                          double(fixedFinish.y)-double(adaptiveFinish.y)) << "\n";
        std::cout << "recorrido_fijo_mediana_ms=" << Median(fixedTraversalTimes)
            << ", recorrido_adaptativo_mediana_ms="
            << Median(adaptiveTraversalTimes) << "\n";
        for (int i = 0; i < Trials; ++i) {
            std::cout << "repeticion=" << i+1
                << ", fijo_ms=" << fixedTimes[i]
                << ", adaptativo_ms=" << adaptiveTimes[i]
                << ", recorrido_fijo_ms=" << fixedTraversalTimes[i]
                << ", recorrido_adaptativo_ms=" << adaptiveTraversalTimes[i]
                << "\n";
        }
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
