#ifndef THREAD_POOL_HPP
#define THREAD_POOL_HPP

#include "work_stealing_queue.hpp"
#include <vector>
#include <thread>
#include <functional>
#include <atomic>
#include <random>
#include <condition_variable>
#include <memory>

namespace para {

/**
 * Thread Pool with Work-Stealing Scheduler
 *
 * Each worker has its own local queue.
 * When a worker's queue is empty, it tries to steal from other workers.
 */
class ThreadPool {
public:
    using Task = std::function<void()>;
    
    explicit ThreadPool(size_t numThreads = std::thread::hardware_concurrency())
        : running_(true)
        , pendingTasks_(0)
        , stealCount_(0)
    {
        if (numThreads == 0) {
            numThreads = 1;
        }
        
        numWorkers_ = numThreads;
        
        // Create queues using unique_ptr (WorkStealingQueue is not movable)
        for (size_t i = 0; i < numWorkers_; ++i) {
            localQueues_.push_back(std::make_unique<WorkStealingQueue<Task>>());
        }
        
        // Start worker threads
        for (size_t i = 0; i < numWorkers_; ++i) {
            workers_.emplace_back(&ThreadPool::workerFunction, this, i);
        }
    }
    
    ~ThreadPool() {
        shutdown();
    }
    
    // Non-copyable
    ThreadPool(const ThreadPool&) = delete;
    ThreadPool& operator=(const ThreadPool&) = delete;
    
    /**
     * Submit a task to the pool
     * Uses round-robin to distribute to worker queues
     */
    void submit(Task task) {
        if (!running_) return;
        
        pendingTasks_.fetch_add(1, std::memory_order_relaxed);
        
        // Round-robin distribution
        size_t idx = nextQueue_.fetch_add(1, std::memory_order_relaxed) % numWorkers_;
        localQueues_[idx]->pushBack(std::move(task));
        
        // Wake up a worker
        cv_.notify_one();
    }
    
    /**
     * Submit a task to a specific worker's queue
     */
    void submitTo(size_t workerId, Task task) {
        if (!running_ || workerId >= numWorkers_) return;
        
        pendingTasks_.fetch_add(1, std::memory_order_relaxed);
        localQueues_[workerId]->pushBack(std::move(task));
        cv_.notify_one();
    }
    
    /**
     * Wait for all submitted tasks to complete
     */
    void waitAll() {
        std::unique_lock<std::mutex> lock(waitMutex_);
        waitCv_.wait(lock, [this]() {
            return pendingTasks_.load(std::memory_order_relaxed) == 0;
        });
    }
    
    /**
     * Shutdown the thread pool
     */
    void shutdown() {
        if (!running_.exchange(false)) return;
        
        cv_.notify_all();
        
        for (auto& worker : workers_) {
            if (worker.joinable()) {
                worker.join();
            }
        }
    }
    
    /**
     * Get number of workers
     */
    size_t numWorkers() const { return numWorkers_; }
    
    /**
     * Get total steal count (for statistics)
     */
    size_t getStealCount() const { 
        return stealCount_.load(std::memory_order_relaxed); 
    }

private:
    void workerFunction(size_t workerId) {
        // Random generator for victim selection
        std::mt19937 rng(static_cast<unsigned int>(workerId));
        std::uniform_int_distribution<size_t> dist(0, numWorkers_ - 1);
        
        while (running_) {
            Task task;
            bool found = false;
            
            // 1. Try to get task from local queue
            if (auto opt = localQueues_[workerId]->tryPopBack()) {
                task = std::move(*opt);
                found = true;
            }
            
            // 2. Try to steal from other workers
            if (!found) {
                for (size_t attempts = 0; attempts < numWorkers_ * 2; ++attempts) {
                    size_t victim = dist(rng);
                    if (victim != workerId) {
                        if (auto opt = localQueues_[victim]->tryPopFront()) {
                            task = std::move(*opt);
                            found = true;
                            stealCount_.fetch_add(1, std::memory_order_relaxed);
                            break;
                        }
                    }
                }
            }
            
            // 3. Execute task if found
            if (found) {
                task();
                
                // Decrement pending count and notify waiters
                if (pendingTasks_.fetch_sub(1, std::memory_order_relaxed) == 1) {
                    waitCv_.notify_all();
                }
            } else {
                // 4. No work found, wait for new tasks
                std::unique_lock<std::mutex> lock(waitMutex_);
                cv_.wait_for(lock, std::chrono::microseconds(100), [this]() {
                    return !running_ || pendingTasks_.load(std::memory_order_relaxed) > 0;
                });
            }
        }
    }

private:
    size_t numWorkers_;
    std::vector<std::thread> workers_;
    std::vector<std::unique_ptr<WorkStealingQueue<Task>>> localQueues_;
    
    std::atomic<bool> running_;
    std::atomic<size_t> nextQueue_{0};
    std::atomic<size_t> pendingTasks_;
    std::atomic<size_t> stealCount_;
    
    std::mutex waitMutex_;
    std::condition_variable cv_;
    std::condition_variable waitCv_;
};

} // namespace para

#endif // THREAD_POOL_HPP