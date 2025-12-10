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
     * Generate all random inputs
     */
    void generateInputs();
    
    /**
     * Get all generated inputs
     */
    const std::vector<Input>& getInputs() const;
    
    /**
     * Get client ID
     */
    int getClientId() const;
    
    /**
     * Get match ID this client belongs to
     */
    int getMatchId() const;
    
    /**
     * Get player ID (0 or 1)
     */
    int getPlayerId() const;
    
    /**
     * Get number of inputs
     */
    size_t getNumInputs() const;

private:
    int clientId_;
    int matchId_;
    int playerId_;
    int numInputs_;
    
    std::vector<Input> inputs_;
    std::mt19937 rng_;
};

/**
 * ClientManager - Manages all clients for simulation
 */
class ClientManager {
public:
    ClientManager(int numClients = NUM_CLIENTS, int numMatches = NUM_MATCHES, 
                  int inputsPerClient = INPUTS_PER_CLIENT);
    
    /**
     * Generate all inputs for all clients
     */
    void generateAllInputs();
    
    /**
     * Get all inputs from all clients
     */
    std::vector<Input> getAllInputs() const;
    
    /**
     * Get number of clients
     */
    int getNumClients() const;
    
    /**
     * Get total number of inputs
     */
    size_t getTotalInputs() const;

private:
    std::vector<Client> clients_;
    int numClients_;
    int numMatches_;
    int inputsPerClient_;
};

} // namespace para

#endif // CLIENT_HPP