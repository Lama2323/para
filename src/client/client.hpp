#ifndef CLIENT_HPP
#define CLIENT_HPP

#include "../common/types.hpp"
#include "../common/data_structures.hpp"
#include <vector>
#include <random>

namespace para {

/**
 * Client - Simulates a game client that sends inputs
 * 
 * Each client belongs to a specific match and player
 */
class Client {
public:
    Client(int clientId, int matchId, int playerId, int numInputs = INPUTS_PER_CLIENT);
    
    /**
     * Generate inputs for the next batch of ticks
     * Returns a vector of generated inputs
     */
    std::vector<Input> generateBatch(int batchSize);
    
    /**
     * Check if client has finished generating all inputs
     */
    bool isFinished() const;
    
    // Legacy support (optional, can remove if we fully switch)
    // void generateInputs();
    // const std::vector<Input>& getInputs() const;
    
    int getClientId() const;
    int getMatchId() const;
    int getPlayerId() const;
    size_t getNumInputs() const;

private:
    int clientId_;
    int matchId_;
    int playerId_;
    int numInputs_;
    int currentTick_; // Track current generation progress
    
    // std::vector<Input> inputs_; // Removed to save memory in streaming mode
    std::mt19937 rng_;
};

/**
 * ClientManager - Manages all clients for simulation
 */
class ClientManager {
public:
    ClientManager(int numClients = NUM_CLIENTS, int numMatches = NUM_MATCHES, 
                  int inputsPerClient = INPUTS_PER_CLIENT);
    
    // Legacy: Generate all inputs for all clients
    // void generateAllInputs();
    // std::vector<Input> getAllInputs() const;
    
    Client* getClient(int index);

    int getNumClients() const;
    size_t getTotalInputs() const;

private:
    std::vector<Client> clients_;
    int numClients_;
    int numMatches_;
    int inputsPerClient_;
};

} // namespace para

#endif // CLIENT_HPP