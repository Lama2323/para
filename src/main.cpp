#include "common/types.hpp"
#include "common/data_structures.hpp"
#include "scheduler/thread_pool.hpp"
#include "game/game_server.hpp"
#include "client/client.hpp"
#include "tasks/client_task.hpp"
#include "tasks/match_task.hpp"

#include <iostream>
#include <chrono>
#include <iomanip>
#include <atomic>

using namespace para;
using namespace std::chrono;

/**
 * Benchmark result structure
 */
struct BenchmarkResult {
    double timeMs;
    size_t processedInputs;
    int rollbackCount;
    size_t workSteals;
};

/**
 * Run sequential benchmark
 * Generates all inputs first, then processes them sequentially
 */
BenchmarkResult runSequentialBenchmark() {
    BenchmarkResult result = {};
    
    std::cout << "  [Sequential] Generating inputs..." << std::endl;
    
    // 1. Generate all inputs first (to match original behavior)
    ClientManager clientManager(NUM_CLIENTS, NUM_MATCHES, INPUTS_PER_CLIENT);
    std::vector<Input> allInputs;
    allInputs.reserve(NUM_CLIENTS * INPUTS_PER_CLIENT);
    
    // Generate batches until finished
    // Generate batches in round-robin fashion to simulate interleaved arrival 
    // This provides a fairer comparison by ordering inputs roughly by time
    bool anyActive = true;
    while (anyActive) {
        anyActive = false;
        for (int i = 0; i < NUM_CLIENTS; ++i) {
            Client* client = clientManager.getClient(i);
            if (!client->isFinished()) {
                // Use smaller batch size (same as parallel) to interleave inputs finely
                auto batch = client->generateBatch(50);
                allInputs.insert(allInputs.end(), batch.begin(), batch.end());
                
                if (!client->isFinished()) {
                    anyActive = true;
                }
            }
        }
    }
    
    std::cout << "  [Sequential] Processing " << allInputs.size() << " inputs..." << std::endl;

    // 2. Process
    GameServer server(NUM_MATCHES);
    server.start();
    server.receiveInputs(allInputs);
    
    auto start = high_resolution_clock::now();
    server.processAllSequential();
    auto end = high_resolution_clock::now();
    
    result.timeMs = duration_cast<microseconds>(end - start).count() / 1000.0;
    result.processedInputs = server.getProcessedCount();
    result.rollbackCount = server.getTotalRollbackCount();
    result.workSteals = 0;
    
    return result;
}

/**
 * Run task-based concurrent benchmark
 * Pipeline: Client Task (Gen) -> Server (Queue) -> Match Task (Process)
 */
BenchmarkResult runConcurrentBenchmark(size_t numThreads) {
    BenchmarkResult result = {};
    
    GameServer server(NUM_MATCHES);
    ThreadPool pool(numThreads);
    server.start();
    
    ClientManager clientManager(NUM_CLIENTS, NUM_MATCHES, INPUTS_PER_CLIENT);
    
    std::atomic<int> clientsFinished{0};
    
    auto start = high_resolution_clock::now();
    
    // 1. Submit initial Client Tasks
    for (int i = 0; i < NUM_CLIENTS; ++i) {
        pool.submit([&clientManager, &server, &pool, &clientsFinished, i]() {
            // Use modular ClientTask
            ClientTask{clientManager.getClient(i), &server, &pool, &clientsFinished}();
        });
    }
    
    // 2. Submit Match Processing Tasks
    // These run continuously until all clients are done AND queues are empty
    for (int i = 0; i < NUM_MATCHES; ++i) {
        pool.submit([&server, &pool, &clientsFinished, i]() {
            // Use modular MatchTask
            MatchTask(i, &server, &pool, &clientsFinished, NUM_CLIENTS)();
        });
    }
    
    // Wait for everything to drain
    pool.waitAll();
    
    auto end = high_resolution_clock::now();
    
    result.timeMs = duration_cast<microseconds>(end - start).count() / 1000.0;
    result.processedInputs = server.getProcessedCount();
    result.rollbackCount = server.getTotalRollbackCount();
    result.workSteals = pool.getStealCount();
    
    return result;
}

void printSeparator() {
    std::cout << std::string(50, '=') << std::endl;
}

int main() {
    std::cout << std::fixed << std::setprecision(2);
    
    printSeparator();
    std::cout << "  GAME SERVER SIMULATION - TASK PIPELINE DEMO" << std::endl;
    printSeparator();
    
    // Configuration
    std::cout << "\n[Configuration]" << std::endl;
    std::cout << "  Matches:          " << NUM_MATCHES << std::endl;
    std::cout << "  Clients:          " << NUM_CLIENTS << std::endl;
    std::cout << "  Inputs/Client:    " << INPUTS_PER_CLIENT << std::endl;
    std::cout << "  Total Inputs:     " << TOTAL_INPUTS << std::endl;
    std::cout << "  Rollback Every:   " << ROLLBACK_INTERVAL << " ticks" << std::endl;
    std::cout << "  Hardware Threads: " << std::thread::hardware_concurrency() << std::endl;
    
    // Sequential Benchmark
    printSeparator();
    std::cout << "  SEQUENTIAL MODE (Baseline)" << std::endl;
    printSeparator();
    
    BenchmarkResult seqResult = runSequentialBenchmark();
    
    std::cout << "  Time:        " << seqResult.timeMs << " ms" << std::endl;
    std::cout << "  Processed:   " << seqResult.processedInputs << " inputs" << std::endl;
    std::cout << "  Rollbacks:   " << seqResult.rollbackCount << std::endl;
    std::cout << "  Throughput:  " << (seqResult.processedInputs / seqResult.timeMs * 1000) 
              << " inputs/sec" << std::endl;
    
    // Parallel Benchmarks with different thread counts
    std::vector<size_t> threadCounts = {2, 3, 4, 5, 6, 7, 8};
    
    // Safety check just in case running on a machine with fewer threads, though usually fine to oversubscribe for test
    size_t hwThreads = std::thread::hardware_concurrency();
    std::cout << "  [Info] Hardware Threads: " << hwThreads << std::endl;
    
    std::vector<BenchmarkResult> parallelResults;
    
    for (size_t numThreads : threadCounts) {
        printSeparator();
        std::cout << "  PARALLEL PIPELINE TASK MODE (" << numThreads << " threads)" << std::endl;
        printSeparator();
        
        BenchmarkResult parResult = runConcurrentBenchmark(numThreads);
        parallelResults.push_back(parResult);
        
        double speedup = seqResult.timeMs / parResult.timeMs;
        
        std::cout << "  Time:        " << parResult.timeMs << " ms" << std::endl;
        std::cout << "  Processed:   " << parResult.processedInputs << " inputs" << std::endl;
        std::cout << "  Rollbacks:   " << parResult.rollbackCount << std::endl;
        std::cout << "  Work Steals: " << parResult.workSteals << std::endl;
        std::cout << "  Throughput:  " << (parResult.processedInputs / parResult.timeMs * 1000) 
                  << " inputs/sec" << std::endl;
        std::cout << "  Speedup:     " << speedup << "x" << std::endl;
    }
    
    // Summary
    printSeparator();
    std::cout << "  SUMMARY" << std::endl;
    printSeparator();
    
    std::cout << "\n  Mode            | Time (ms) | Speedup | Steals" << std::endl;
    std::cout << "  ----------------|-----------|---------|-------" << std::endl;
    std::cout << "  Sequential      | " << std::setw(9) << seqResult.timeMs 
              << " | " << std::setw(7) << "1.00x" << " | " << std::setw(6) << "N/A" << std::endl;
    
    for (size_t i = 0; i < threadCounts.size(); ++i) {
        double speedup = seqResult.timeMs / parallelResults[i].timeMs;
        std::cout << "  Parallel (" << std::setw(2) << threadCounts[i] << "T)  | " 
                  << std::setw(9) << parallelResults[i].timeMs 
                  << " | " << std::setw(6) << speedup << "x | " 
                  << std::setw(6) << parallelResults[i].workSteals << std::endl;
    }
    
    std::cout << "\n";
    printSeparator();
    std::cout << "  DEMO COMPLETE" << std::endl;
    printSeparator();
    std::cout << "Press any key to exit..." << std::endl;
    std::cin.get();
    return 0;
}