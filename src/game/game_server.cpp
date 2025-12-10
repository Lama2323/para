#include "game_server.hpp"
#include <algorithm>

namespace para {

GameServer::GameServer(int numMatches) 
    : numMatches_(numMatches)
{
    // Create all matches
    matches_.reserve(numMatches);
    for (int i = 0; i < numMatches; ++i) {
        matches_.push_back(std::make_unique<Match>(i));
    }
}

void GameServer::start() {
    // Start all matches
    for (auto& match : matches_) {
        match->start();
    }
}

void GameServer::receiveInput(const Input& input) {
    std::lock_guard<std::mutex> lock(queueMutex_);
    inputQueue_.push(input);
}

void GameServer::receiveInputs(const std::vector<Input>& inputs) {
    std::lock_guard<std::mutex> lock(queueMutex_);
    for (const auto& input : inputs) {
        inputQueue_.push(input);
    }
}

void GameServer::processAllSequential() {
    // Process all inputs in a single thread
    while (true) {
        Input input;
        
        // Get next input from queue
        {
            std::lock_guard<std::mutex> lock(queueMutex_);
            if (inputQueue_.empty()) break;
            input = inputQueue_.front();
            inputQueue_.pop();
        }
        
        // Route to correct match
        processSingleInput(input);
    }
}

void GameServer::processAllParallel(ThreadPool& pool) {
    // Distribute inputs to thread pool
    // Each match's inputs are grouped together for better locality
    
    std::vector<std::vector<Input>> inputsByMatch(numMatches_);
    
    // Group inputs by match
    {
        std::lock_guard<std::mutex> lock(queueMutex_);
        while (!inputQueue_.empty()) {
            Input input = inputQueue_.front();
            inputQueue_.pop();
            
            if (input.matchId >= 0 && input.matchId < numMatches_) {
                inputsByMatch[input.matchId].push_back(input);
            }
        }
    }
    
    // Submit processing tasks for each match
    for (int matchId = 0; matchId < numMatches_; ++matchId) {
        if (inputsByMatch[matchId].empty()) continue;
        
        // Capture by value to avoid data races
        std::vector<Input> inputs = std::move(inputsByMatch[matchId]);
        Match* match = matches_[matchId].get();
        
        pool.submit([this, match, inputs = std::move(inputs)]() {
            for (const auto& input : inputs) {
                match->processInput(input);
                processedCount_.fetch_add(1, std::memory_order_relaxed);
            }
        });
    }
    
    // Wait for all tasks to complete
    pool.waitAll();
}

void GameServer::processSingleInput(const Input& input) {
    if (input.matchId < 0 || input.matchId >= numMatches_) {
        return;
    }
    
    matches_[input.matchId]->processInput(input);
    processedCount_.fetch_add(1, std::memory_order_relaxed);
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
    std::lock_guard<std::mutex> lock(queueMutex_);
    return inputQueue_.size();
}

bool GameServer::isAllProcessed() const {
    std::lock_guard<std::mutex> lock(queueMutex_);
    return inputQueue_.empty();
}

int GameServer::getNumMatches() const {
    return numMatches_;
}

void GameServer::clearInputs() {
    std::lock_guard<std::mutex> lock(queueMutex_);
    while (!inputQueue_.empty()) {
        inputQueue_.pop();
    }
    processedCount_.store(0, std::memory_order_relaxed);
}

} // namespace para