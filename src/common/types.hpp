#ifndef TYPES_HPP
#define TYPES_HPP

#include <cstdint>

namespace para {

// ============================================
// Action Types - Simplified (Movement only)
// ============================================
enum class ActionType : uint8_t {
    MOVE_LEFT,
    MOVE_RIGHT,
    MOVE_UP,
    MOVE_DOWN
};

// ============================================
// Game Constants
// ============================================
constexpr int ARENA_WIDTH = 20;
constexpr int ARENA_HEIGHT = 20;
constexpr int ROLLBACK_INTERVAL = 5;  // Rollback every 5 ticks

// ============================================
// Simulation Constants
// ============================================
constexpr int NUM_MATCHES = 20;
constexpr int NUM_CLIENTS = 40;
constexpr int INPUTS_PER_CLIENT = 10000;
constexpr int TOTAL_INPUTS = NUM_CLIENTS * INPUTS_PER_CLIENT;  // 400,000

// ============================================
// Helper Functions
// ============================================
inline const char* actionTypeToString(ActionType type) {
    switch (type) {
        case ActionType::MOVE_LEFT:  return "MOVE_LEFT";
        case ActionType::MOVE_RIGHT: return "MOVE_RIGHT";
        case ActionType::MOVE_UP:    return "MOVE_UP";
        case ActionType::MOVE_DOWN:  return "MOVE_DOWN";
        default: return "UNKNOWN";
    }
}

} // namespace para

#endif // TYPES_HPP