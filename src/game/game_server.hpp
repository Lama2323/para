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
    
    // Non-copyable
    GameServer(const GameServer&) = delete;
    GameServer& operator=(const GameServer&) = delete;
    
    /**
     * Initialize and start all matches
     */
    void start();
    
    /**
     * Receive input and add to queue (thread-safe)
     */
    void receiveInput(const Input& input);
    
    /**
     * Receive multiple inputs at once
     */
    void receiveInputs(const std::vector<Input>& inputs);
    
    /**
     * Process all inputs SEQUENTIALLY (single-threaded)
     */
    void processAllSequential();
    
    /**
     * Process all inputs in PARALLEL (using thread pool)
     */
    void processAllParallel(ThreadPool& pool);
    
    /**
     * Process single input - route to correct match
     */
    void processSingleInput(const Input& input);
    
    /**
     * Get total number of processed inputs
     */
    size_t getProcessedCount() const;
    
    /**
     * Get total rollback count across all matches
     */
    int getTotalRollbackCount() const;
    
    /**
     * Get pending input count
     */
    size_t getPendingCount() const;
    
    /**
     * Check if all inputs processed
     */
    bool isAllProcessed() const;
    
    /**
     * Get number of matches
     */
    int getNumMatches() const;
    
    /**
     * Clear input queue (for reuse)
     */
    void clearInputs();

private:
    std::vector<std::unique_ptr<Match>> matches_;
    std::queue<Input> inputQueue_;
    mutable std::mutex queueMutex_;
    
    std::atomic<size_t> processedCount_{0};
    int numMatches_;
};

} // namespace para

#endif // GAME_SERVER_HPP