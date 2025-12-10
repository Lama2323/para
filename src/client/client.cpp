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
    , rng_(clientId)  // Seed with clientId for reproducible results
{
    inputs_.reserve(numInputs);
}

void Client::generateInputs() {
    inputs_.clear();
    inputs_.reserve(numInputs_);
    
    std::uniform_int_distribution<int> actionDist(0, 3);  // 4 movement actions
    
    for (int i = 0; i < numInputs_; ++i) {
        Input input;
        input.matchId = matchId_;
        input.playerId = playerId_;
        input.tickId = i;  // Each input at different tick
        input.type = static_cast<ActionType>(actionDist(rng_));
        
        inputs_.push_back(input);
    }
}

const std::vector<Input>& Client::getInputs() const {
    return inputs_;
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
    return inputs_.size();
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
    
    // Create clients - 2 clients per match
    // Client 0, 1 -> Match 0
    // Client 2, 3 -> Match 1
    // etc.
    for (int i = 0; i < numClients; ++i) {
        int matchId = i / 2;  // 2 clients per match
        int playerId = i % 2; // Player 0 or 1
        
        // Ensure matchId is within bounds
        if (matchId >= numMatches) {
            matchId = matchId % numMatches;
        }
        
        clients_.emplace_back(i, matchId, playerId, inputsPerClient);
    }
}

void ClientManager::generateAllInputs() {
    for (auto& client : clients_) {
        client.generateInputs();
    }
}

std::vector<Input> ClientManager::getAllInputs() const {
    std::vector<Input> allInputs;
    allInputs.reserve(numClients_ * inputsPerClient_);
    
    // Interleave inputs from all clients to simulate real concurrent sends
    // This creates a more realistic input pattern
    
    for (int tick = 0; tick < inputsPerClient_; ++tick) {
        for (const auto& client : clients_) {
            const auto& inputs = client.getInputs();
            if (tick < static_cast<int>(inputs.size())) {
                allInputs.push_back(inputs[tick]);
            }
        }
    }
    
    return allInputs;
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