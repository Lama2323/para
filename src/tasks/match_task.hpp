#ifndef MATCH_TASK_HPP
#define MATCH_TASK_HPP

#include "../game/game_server.hpp"
#include "../scheduler/thread_pool.hpp"
#include <atomic>

namespace para {

/**
 * Task responsible for processing a specific match.
 * Self-replicating: Resubmits itself until all clients are done AND queue is empty.
 */
struct MatchTask {
    int matchId;
    GameServer* server;
    ThreadPool* pool;
    std::atomic<int>* clientsFinished;
    int data_num_clients; // Needed for termination condition check

    // Add constructor to force initialization
    MatchTask(int id, GameServer* srv, ThreadPool* tp, std::atomic<int>* finished, int numClients)
        : matchId(id), server(srv), pool(tp), clientsFinished(finished), data_num_clients(numClients) {}

    void operator()() {
        // Process what's available
        server->processPending(matchId);
        
        // Check termination condition
        bool allClientsDone = clientsFinished->load(std::memory_order_relaxed) == data_num_clients;
        bool queueEmpty = server->getPendingCount() == 0; // Optimization: Could check specific queue
        
        if (!allClientsDone || !queueEmpty) {
            // Keep running
            // ThreadPool handles scheduling, so just resubmit.
            pool->submit(*this);
        }
    }
};

} // namespace para

#endif // MATCH_TASK_HPP
