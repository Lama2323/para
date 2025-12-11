#ifndef GAME_SERVER_HPP
#define GAME_SERVER_HPP

#include "match.hpp"
#include "../common/types.hpp"
#include "../common/data_structures.hpp"
#include "../scheduler/thread_pool.hpp"
#include <vector>
#include <queue>
#include <mutex>
#include <atomic>
#include <memory>

namespace para {

/**
 * GameServer - Manages multiple matches and input routing
 * 
 * Supports both sequential and parallel processing modes
 */
class GameServer {
public:
    explicit GameServer(int numMatches = NUM_MATCHES);
    ~GameServer() = default;
    
    GameServer(const GameServer&) = delete;
    GameServer& operator=(const GameServer&) = delete;
    
    void start();
    
    // Receive input and dispatch to correct match queue
    void receiveInput(const Input& input);
    
    // Receive multiple inputs
    void receiveInputs(const std::vector<Input>& inputs);
    
    // Process pending inputs for a specific match
    void processPending(int matchId);
    
    // Legacy support: Process all inputs SEQUENTIALLY
    void processAllSequential();
    
    // Legacy support: Process all inputs in PARALLEL
    void processAllParallel(ThreadPool& pool);
    
    void processSingleInput(const Input& input);
    
    size_t getProcessedCount() const;
    int getTotalRollbackCount() const;
    
    // Get total pending count across all queues
    size_t getPendingCount() const;
    
    bool isAllProcessed() const;
    int getNumMatches() const;
    void clearInputs();

private:
    struct MatchQueue {
        std::queue<Input> queue;
        std::mutex mutex;
    };

    std::vector<std::unique_ptr<Match>> matches_;
    std::vector<std::unique_ptr<MatchQueue>> matchQueues_;
    
    std::atomic<size_t> processedCount_{0};
    int numMatches_;
};

} // namespace para

#endif // GAME_SERVER_HPP