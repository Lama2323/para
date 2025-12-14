#ifndef CLIENT_TASK_HPP
#define CLIENT_TASK_HPP

#include "../client/client.hpp"
#include "../game/game_server.hpp"
#include "../scheduler/thread_pool.hpp"
#include <atomic>

namespace para {

/**
 * Task responsible for generating inputs for a specific client.
 * Self-replicating: Resubmits itself until client data is exhausted.
 */
struct ClientTask {
    Client* client;
    GameServer* server;
    ThreadPool* pool;
    std::atomic<int>* clientsFinished;
    
    void operator()() {
        // Generate small batch to simulate continuous input
        // Batch size small enough to cause frequent task switching but large enough for efficiency
        auto batch = client->generateBatch(BATCH_SIZE); 
        
        if (!batch.empty()) {
            server->receiveInputs(batch);
        }
        
        if (!client->isFinished()) {
            // Re-submit self
            pool->submit(*this);
        } else {
            clientsFinished->fetch_add(1, std::memory_order_relaxed);
        }
    }
};

} // namespace para

#endif // CLIENT_TASK_HPP
