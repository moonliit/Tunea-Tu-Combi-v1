#ifndef LEADERBOARD_CLIENT_HPP
#define LEADERBOARD_CLIENT_HPP

#include "identity/IdentitySession.hpp"
#include "scoring/ScoreRules.hpp"

#include <cstdint>
#include <string>
#include <vector>

enum class LeaderboardStatus {
    Idle,
    Loading,
    Ready,
    Unavailable
};

struct LeaderboardEntry {
    int rank = 0;
    std::string combiId;
    std::string name;
    std::string attemptId;
    std::string runRef;
    std::int64_t total = 0;
    std::int64_t driverTimeMs = 0;
    std::int64_t initialParts = 0;
};

struct PersonalBest {
    bool available = false;
    bool currentAttempt = false;
    std::int64_t total = 0;
    std::int64_t driverTimeMs = 0;
    std::int64_t initialParts = 0;
};

struct LeaderboardSnapshot {
    LeaderboardStatus status = LeaderboardStatus::Idle;
    std::vector<LeaderboardEntry> entries;
    PersonalBest personalBest;
    bool submissionConfirmed = false;
    bool previousBestRetained = false;
    std::string championRunRef;
    std::string message;
};

class LeaderboardClient {
private:
    LeaderboardSnapshot m_snapshot;
    std::string m_attemptId;
    std::string m_combiId;
    double m_pollDelayMs = 0.0;

public:
    void Submit(
        const std::string& levelId,
        const std::string& attemptId,
        const IdentityScope& scope,
        const Scoring::RunStatistics& statistics,
        const Scoring::ScoreBreakdown& score
    );
    void Poll(double frameMs);
    void Reset();

    const LeaderboardSnapshot& GetSnapshot() const { return m_snapshot; }
    const std::string& GetAttemptId() const { return m_attemptId; }
    const std::string& GetCombiId() const { return m_combiId; }
};

#endif
