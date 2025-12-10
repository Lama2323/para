#include "common/types.hpp"
#include "common/data_structures.hpp"
#include "scheduler/thread_pool.hpp"
#include "game/game_server.hpp"
#include "client/client.hpp"

#include <iostream>
#include <chrono>
#include <iomanip>

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
 */
BenchmarkResult runSequentialBenchmark(const std::vector<Input>& inputs) {
    BenchmarkResult result = {};
    
    // Create fresh server
    GameServer server(NUM_MATCHES);
    server.start();
    
    // Add all inputs to queue
    server.receiveInputs(inputs);
    
    // Measure time
    auto start = high_resolution_clock::now();
    server.processAllSequential();
    auto end = high_resolution_clock::now();
    
    result.timeMs = duration_cast<microseconds>(end - start).count() / 1000.0;
    result.processedInputs = server.getProcessedCount();
    result.rollbackCount = server.getTotalRollbackCount();
    result.workSteals = 0;  // N/A for sequential
    
    return result;
}

/**
 * Run parallel benchmark
 */
BenchmarkResult runParallelBenchmark(const std::vector<Input>& inputs, size_t numThreads) {
    BenchmarkResult result = {};
    
    // Create fresh server and thread pool
    GameServer server(NUM_MATCHES);
    ThreadPool pool(numThreads);
    server.start();
    
    // Add all inputs to queue
    server.receiveInputs(inputs);
    
    // Measure time
    auto start = high_resolution_clock::now();
    server.processAllParallel(pool);
    auto end = high_resolution_clock::now();
    
    result.timeMs = duration_cast<microseconds>(end - start).count() / 1000.0;
    result.processedInputs = server.getProcessedCount();
    result.rollbackCount = server.getTotalRollbackCount();
    result.workSteals = pool.getStealCount();
    
    return result;
}

/**
 * Print separator line
 */
void printSeparator() {
    std::cout << std::string(50, '=') << std::endl;
}

/**
 * Main entry point
 */
int main() {
    std::cout << std::fixed << std::setprecision(2);
    
    printSeparator();
    std::cout << "  GAME SERVER SIMULATION - WORK STEALING DEMO" << std::endl;
    printSeparator();
    
    // Configuration
    std::cout << "\n[Configuration]" << std::endl;
    std::cout << "  Matches:          " << NUM_MATCHES << std::endl;
    std::cout << "  Clients:          " << NUM_CLIENTS << std::endl;
    std::cout << "  Inputs/Client:    " << INPUTS_PER_CLIENT << std::endl;
    std::cout << "  Total Inputs:     " << TOTAL_INPUTS << std::endl;
    std::cout << "  Arena Size:       " << ARENA_WIDTH << "x" << ARENA_HEIGHT << std::endl;
    std::cout << "  Rollback Every:   " << ROLLBACK_INTERVAL << " ticks" << std::endl;
    std::cout << "  Hardware Threads: " << std::thread::hardware_concurrency() << std::endl;
    
    // Generate inputs
    std::cout << "\n[Generating Inputs]" << std::endl;
    auto genStart = high_resolution_clock::now();
    
    ClientManager clientManager(NUM_CLIENTS, NUM_MATCHES, INPUTS_PER_CLIENT);
    clientManager.generateAllInputs();
    std::vector<Input> allInputs = clientManager.getAllInputs();
    
    auto genEnd = high_resolution_clock::now();
    double genTime = duration_cast<milliseconds>(genEnd - genStart).count();
    
    std::cout << "  Generated " << allInputs.size() << " inputs in " << genTime << " ms" << std::endl;
    
    // Sequential Benchmark
    printSeparator();
    std::cout << "  SEQUENTIAL MODE" << std::endl;
    printSeparator();
    
    BenchmarkResult seqResult = runSequentialBenchmark(allInputs);
    
    std::cout << "  Time:        " << seqResult.timeMs << " ms" << std::endl;
    std::cout << "  Processed:   " << seqResult.processedInputs << " inputs" << std::endl;
    std::cout << "  Rollbacks:   " << seqResult.rollbackCount << std::endl;
    std::cout << "  Throughput:  " << (seqResult.processedInputs / seqResult.timeMs * 1000) 
              << " inputs/sec" << std::endl;
    
    // Parallel Benchmarks with different thread counts
    std::vector<size_t> threadCounts = {2, 4};
    size_t hwThreads = std::thread::hardware_concurrency();
    if (hwThreads > 4) {
        threadCounts.push_back(hwThreads / 2);
        threadCounts.push_back(hwThreads);
    }
    
    std::vector<BenchmarkResult> parallelResults;
    
    for (size_t numThreads : threadCounts) {
        printSeparator();
        std::cout << "  PARALLEL MODE (" << numThreads << " threads)" << std::endl;
        printSeparator();
        
        BenchmarkResult parResult = runParallelBenchmark(allInputs, numThreads);
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
    std::cin.get();
    return 0;
}