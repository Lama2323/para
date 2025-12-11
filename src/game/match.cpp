#include "match.hpp"
#include <algorithm>

namespace para {

Match::Match(int matchId) 
    : state_(matchId)
    , lastSnapshotTick_(-ROLLBACK_INTERVAL)
{
}

Match::Match(Match&& other) noexcept
    : state_(std::move(other.state_))
    , snapshots_(std::move(other.snapshots_))
    , inputHistory_(std::move(other.inputHistory_))
    , rollbackCount_(other.rollbackCount_.load())
    , lastSnapshotTick_(other.lastSnapshotTick_)
{
}

Match& Match::operator=(Match&& other) noexcept {
    if (this != &other) {
        std::lock_guard<std::mutex> lock(mutex_);
        state_ = std::move(other.state_);
        snapshots_ = std::move(other.snapshots_);
        inputHistory_ = std::move(other.inputHistory_);
        rollbackCount_.store(other.rollbackCount_.load());
        lastSnapshotTick_ = other.lastSnapshotTick_;
    }
    return *this;
}

void Match::start() {
    std::lock_guard<std::mutex> lock(mutex_);
    state_.isRunning = true;
    state_.currentTick = 0;
    
    // Save initial snapshot
    saveSnapshot();
}

void Match::processInput(const Input& input) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (!state_.isRunning) return;
    
    // Store input in history
    inputHistory_.push_back(input);
    
    // Check if this is a late input (needs rollback)
    if (input.tickId < state_.currentTick) {
        // Late input - need to rollback
        rollbackCount_.fetch_add(1, std::memory_order_relaxed);
        
        // Find snapshot and re-simulate
        const Snapshot* snapshot = findSnapshotForTick(input.tickId);
        if (snapshot) {
            // Load snapshot state
            state_ = snapshot->state.clone();
            
            // Re-apply all inputs from snapshot tick to current
            for (const auto& histInput : inputHistory_) {
                if (histInput.tickId >= snapshot->tickId) {
                    applyInput(histInput);
                }
            }
        }
    } else {
        // Normal input
        applyInput(input);
    }
    
    // Advance tick
    advanceTick();
    
    // Save snapshot every ROLLBACK_INTERVAL ticks
    if (state_.currentTick - lastSnapshotTick_ >= ROLLBACK_INTERVAL) {
        saveSnapshot();
        lastSnapshotTick_ = state_.currentTick;
        
        // Force a demo rollback every 5 ticks as per spec
        if (state_.currentTick > 0 && !inputHistory_.empty()) {
            // Simulate a rollback by going back 2 ticks
            int rollbackTick = std::max(0, state_.currentTick - 2);
            rollbackCount_.fetch_add(1, std::memory_order_relaxed);
            
            const Snapshot* snap = findSnapshotForTick(rollbackTick);
            if (snap) {
                state_ = snap->state.clone();
                // Re-simulate
                for (const auto& histInput : inputHistory_) {
                    if (histInput.tickId >= snap->tickId && histInput.tickId <= state_.currentTick) {
                        applyInput(histInput);
                    }
                }
            }
        }
    }
}

void Match::applyInput(const Input& input) {
    // Get the player (0 or 1)
    int playerIdx = input.playerId % 2;
    PlayerState& player = state_.players[playerIdx];
    
    // Apply movement
    player.move(input.type);
}

void Match::saveSnapshot() {
    snapshots_.emplace_back(state_.currentTick, state_);
    
    // Keep only last 10 snapshots to limit memory
    if (snapshots_.size() > 10) {
        snapshots_.erase(snapshots_.begin());
        
        // Pruning Optimization: Remove inputs older than the oldest snapshot
        // We never rollback further back than the oldest snapshot, so these inputs are useless
        if (!snapshots_.empty()) {
            int oldestTick = snapshots_.front().tickId;
            
            // Remove inputs older than oldestTick
            // Since inputHistory_ is append-only by arrival time, it's NOT strictly sorted by tickId
            // (due to late inputs), so we must check all or use remove_if.
            // However, it's roughly sorted, so this linear scan is acceptable if we keep the size small.
            auto it = std::remove_if(inputHistory_.begin(), inputHistory_.end(), 
                [oldestTick](const Input& input) {
                    return input.tickId < oldestTick;
                });
            
            if (it != inputHistory_.end()) {
                inputHistory_.erase(it, inputHistory_.end());
            }
        }
    }
}

void Match::rollback(int toTick) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    const Snapshot* snapshot = findSnapshotForTick(toTick);
    if (!snapshot) return;
    
    rollbackCount_.fetch_add(1, std::memory_order_relaxed);
    
    // Load snapshot
    state_ = snapshot->state.clone();
    
    // Re-simulate from snapshot to current
    int targetTick = state_.currentTick;
    for (const auto& input : inputHistory_) {
        if (input.tickId >= snapshot->tickId && input.tickId <= targetTick) {
            applyInput(input);
        }
    }
}

const Snapshot* Match::findSnapshotForTick(int tick) const {
    if (snapshots_.empty()) return nullptr;
    
    // Find the snapshot with tickId <= tick
    const Snapshot* best = nullptr;
    for (const auto& snap : snapshots_) {
        if (snap.tickId <= tick) {
            best = &snap;
        } else {
            break;
        }
    }
    
    return best ? best : &snapshots_.front();
}

void Match::advanceTick() {
    state_.currentTick++;
}

int Match::getCurrentTick() const {
    return state_.currentTick;
}

int Match::getRollbackCount() const {
    return rollbackCount_.load(std::memory_order_relaxed);
}

int Match::getMatchId() const {
    return state_.matchId;
}

bool Match::isRunning() const {
    return state_.isRunning;
}

MatchState Match::getState() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return state_.clone();
}

} // namespace para