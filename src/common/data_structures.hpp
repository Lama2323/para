#ifndef DATA_STRUCTURES_HPP
#define DATA_STRUCTURES_HPP

#include "types.hpp"
#include <array>
#include <vector>
#include <algorithm>

namespace para {

// ============================================
// Input - Command from client to server
// ============================================
struct Input {
    int matchId;      // Which match this input belongs to
    int playerId;     // Which player sent this input
    int tickId;       // Game tick when this input was generated
    ActionType type;  // MOVE_LEFT, MOVE_RIGHT, MOVE_UP, MOVE_DOWN
    
    Input() = default;
    Input(int match, int player, int tick, ActionType action)
        : matchId(match), playerId(player), tickId(tick), type(action) {}
};

// ============================================
// PlayerState - State of a single player
// ============================================
struct PlayerState {
    int id;
    int x;  // Position x [0, ARENA_WIDTH-1]
    int y;  // Position y [0, ARENA_HEIGHT-1]
    
    PlayerState() : id(0), x(ARENA_WIDTH / 2), y(ARENA_HEIGHT / 2) {}
    PlayerState(int playerId) : id(playerId), x(ARENA_WIDTH / 2), y(ARENA_HEIGHT / 2) {}
    
    // Apply movement with boundary clamping
    void move(ActionType action) {
        switch (action) {
            case ActionType::MOVE_LEFT:
                x = std::max(0, x - 1);
                break;
            case ActionType::MOVE_RIGHT:
                x = std::min(ARENA_WIDTH - 1, x + 1);
                break;
            case ActionType::MOVE_UP:
                y = std::max(0, y - 1);
                break;
            case ActionType::MOVE_DOWN:
                y = std::min(ARENA_HEIGHT - 1, y + 1);
                break;
        }
    }
    
    // Clone for snapshot
    PlayerState clone() const {
        PlayerState copy;
        copy.id = id;
        copy.x = x;
        copy.y = y;
        return copy;
    }
};

// ============================================
// MatchState - State of a single match
// ============================================
struct MatchState {
    int matchId;
    int currentTick;
    std::array<PlayerState, 2> players;  // 2 players per match
    bool isRunning;
    
    MatchState() : matchId(0), currentTick(0), isRunning(false) {}
    MatchState(int id) : matchId(id), currentTick(0), isRunning(false) {
        players[0] = PlayerState(0);
        players[1] = PlayerState(1);
        // Start players at different positions
        players[0].x = 5;
        players[0].y = 10;
        players[1].x = 15;
        players[1].y = 10;
    }
    
    // Clone for snapshot
    MatchState clone() const {
        MatchState copy;
        copy.matchId = matchId;
        copy.currentTick = currentTick;
        copy.players[0] = players[0].clone();
        copy.players[1] = players[1].clone();
        copy.isRunning = isRunning;
        return copy;
    }
};

// ============================================
// Snapshot - State snapshot for rollback
// ============================================
struct Snapshot {
    int tickId;
    MatchState state;
    
    Snapshot() : tickId(0) {}
    Snapshot(int tick, const MatchState& s) : tickId(tick), state(s.clone()) {}
};

} // namespace para

#endif // DATA_STRUCTURES_HPP