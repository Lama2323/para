#include "client.hpp"

namespace para {

// ============================================
// Client Implementation
// ============================================

Client::Client(int clientId, int matchId, int playerId, int numInputs)
    : clientId_(clientId)
    , matchId_(matchId)
    , playerId_(playerId)
    , numInputs_(numInputs)
    , currentTick_(0)
    , rng_(clientId)  // Seed with clientId for reproducible results
{
}

std::vector<Input> Client::generateBatch(int batchSize) {
    std::vector<Input> batch;
    batch.reserve(batchSize);
    
    std::uniform_int_distribution<int> actionDist(0, 3);
    
    int endTick = std::min(currentTick_ + batchSize, numInputs_);
    
    for (int i = currentTick_; i < endTick; ++i) {
        Input input;
        input.matchId = matchId_;
        input.playerId = playerId_;
        input.tickId = i;
        input.type = static_cast<ActionType>(actionDist(rng_));
        
        batch.push_back(input);
    }
    
    currentTick_ = endTick;
    return batch;
}

bool Client::isFinished() const {
    return currentTick_ >= numInputs_;
}

int Client::getClientId() const {
    return clientId_;
}

int Client::getMatchId() const {
    return matchId_;
}

int Client::getPlayerId() const {
    return playerId_;
}

size_t Client::getNumInputs() const {
    return numInputs_;
}

// ============================================
// ClientManager Implementation
// ============================================

ClientManager::ClientManager(int numClients, int numMatches, int inputsPerClient)
    : numClients_(numClients)
    , numMatches_(numMatches)
    , inputsPerClient_(inputsPerClient)
{
    clients_.reserve(numClients);
    
    for (int i = 0; i < numClients; ++i) {
        int matchId = i / 2;
        int playerId = i % 2;
        
        if (matchId >= numMatches) {
            matchId = matchId % numMatches;
        }
        
        clients_.emplace_back(i, matchId, playerId, inputsPerClient);
    }
}

Client* ClientManager::getClient(int index) {
    if (index >= 0 && index < static_cast<int>(clients_.size())) {
        return &clients_[index];
    }
    return nullptr;
}

int ClientManager::getNumClients() const {
    return numClients_;
}

size_t ClientManager::getTotalInputs() const {
    size_t total = 0;
    for (const auto& client : clients_) {
        total += client.getNumInputs();
    }
    return total;
}

} // namespace para