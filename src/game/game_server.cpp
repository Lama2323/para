#include "game_server.hpp"
#include <algorithm>

namespace para {

GameServer::GameServer(int numMatches) 
    : numMatches_(numMatches)
{
    // Create all matches and queues
    matches_.reserve(numMatches);
    matchQueues_.reserve(numMatches);
    for (int i = 0; i < numMatches; ++i) {
        matches_.push_back(std::make_unique<Match>(i));
        matchQueues_.push_back(std::make_unique<MatchQueue>());
    }
}

void GameServer::start() {
    for (auto& match : matches_) {
        match->start();
    }
}

void GameServer::receiveInput(const Input& input) {
    if (input.matchId >= 0 && input.matchId < numMatches_) {
        std::lock_guard<std::mutex> lock(matchQueues_[input.matchId]->mutex);
        matchQueues_[input.matchId]->queue.push(input);
    }
}

void GameServer::receiveInputs(const std::vector<Input>& inputs) {
    for (const auto& input : inputs) {
        receiveInput(input);
    }
}

void GameServer::processPending(int matchId) {
    if (matchId < 0 || matchId >= numMatches_) return;
    
    std::queue<Input> localQueue;
    
    // Extract everything currently in the queue
    {
        std::lock_guard<std::mutex> lock(matchQueues_[matchId]->mutex);
        if (matchQueues_[matchId]->queue.empty()) return;
        std::swap(localQueue, matchQueues_[matchId]->queue);
    }
    
    Match* match = matches_[matchId].get();
    
    // Process without holding the lock
    while (!localQueue.empty()) {
        const Input& input = localQueue.front();
        match->processInput(input);
        localQueue.pop();
        processedCount_.fetch_add(1, std::memory_order_relaxed);
    }
}

void GameServer::processAllSequential() {
    bool hasWork = true;
    while (hasWork) {
        hasWork = false;
        for (int i = 0; i < numMatches_; ++i) {
            {
                std::lock_guard<std::mutex> lock(matchQueues_[i]->mutex);
                if (!matchQueues_[i]->queue.empty()) {
                    hasWork = true;
                }
            }
            if (hasWork) {
                processPending(i);
            }
        }
    }
}

void GameServer::processAllParallel(ThreadPool& pool) {
    // Just submit a task for each match to process its queue
    for (int i = 0; i < numMatches_; ++i) {
        pool.submit([this, i]() {
            processPending(i);
        });
    }
    pool.waitAll();
}

void GameServer::processSingleInput(const Input& input) {
    // Legacy helper - mostly redundant now but keeping for interface compatibility if needed internally
    if (input.matchId >= 0 && input.matchId < numMatches_) {
        matches_[input.matchId]->processInput(input);
        processedCount_.fetch_add(1, std::memory_order_relaxed);
    }
}

size_t GameServer::getProcessedCount() const {
    return processedCount_.load(std::memory_order_relaxed);
}

int GameServer::getTotalRollbackCount() const {
    int total = 0;
    for (const auto& match : matches_) {
        total += match->getRollbackCount();
    }
    return total;
}

size_t GameServer::getPendingCount() const {
    size_t total = 0;
    for (const auto& mq : matchQueues_) {
        std::lock_guard<std::mutex> lock(mq->mutex);
        total += mq->queue.size();
    }
    return total;
}

bool GameServer::isAllProcessed() const {
    return getPendingCount() == 0;
}

int GameServer::getNumMatches() const {
    return numMatches_;
}

void GameServer::clearInputs() {
    for (const auto& mq : matchQueues_) {
        std::lock_guard<std::mutex> lock(mq->mutex);
        std::queue<Input> empty;
        std::swap(mq->queue, empty);
    }
    processedCount_.store(0, std::memory_order_relaxed);
}

} // namespace para