#ifndef MATCH_HPP
#define MATCH_HPP

#include "../common/types.hpp"
#include "../common/data_structures.hpp"
#include <vector>
#include <mutex>
#include <atomic>

namespace para {

/**
 * Match - Represents a single game match
 * 
 * Handles:
 * - Processing player inputs
 * - Managing game state
 * - Snapshots for rollback
 * - Rollback and re-simulation
 */
class Match {
public:
    explicit Match(int matchId);
    
    // Non-copyable (due to mutex)
    Match(const Match&) = delete;
    Match& operator=(const Match&) = delete;
    
    // Movable for container use
    Match(Match&& other) noexcept;
    Match& operator=(Match&& other) noexcept;
    
    /**
     * Start the match
     */
    void start();
    
    /**
     * Process an input from a client
     * Will trigger rollback if input is late
     */
    void processInput(const Input& input);
    
    /**
     * Apply input directly to state (internal)
     */
    void applyInput(const Input& input);
    
    /**
     * Save current state as snapshot
     */
    void saveSnapshot();
    
    /**
     * Rollback to a specific tick and re-simulate
     */
    void rollback(int toTick);
    
    /**
     * Get current tick
     */
    int getCurrentTick() const;
    
    /**
     * Get rollback count (for statistics)
     */
    int getRollbackCount() const;
    
    /**
     * Get match ID
     */
    int getMatchId() const;
    
    /**
     * Check if match is running
     */
    bool isRunning() const;
    
    /**
     * Get current state (thread-safe copy)
     */
    MatchState getState() const;

private:
    /**
     * Find the best snapshot for rollback
     */
    const Snapshot* findSnapshotForTick(int tick) const;
    
    /**
     * Advance to next tick
     */
    void advanceTick();

private:
    MatchState state_;
    std::vector<Snapshot> snapshots_;
    std::vector<Input> inputHistory_;
    
    mutable std::mutex mutex_;
    std::atomic<int> rollbackCount_{0};
    
    int lastSnapshotTick_ = -ROLLBACK_INTERVAL;
};

} // namespace para

#endif // MATCH_HPP