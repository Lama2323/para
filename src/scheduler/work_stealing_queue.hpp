#ifndef WORK_STEALING_QUEUE_HPP
#define WORK_STEALING_QUEUE_HPP

#include <deque>
#include <mutex>
#include <atomic>
#include <optional>

namespace para {

/**
 * Work-Stealing Queue
 * 
 * Thread-safe deque that supports:
 * - pushBack / popBack: Used by owner thread (LIFO for better cache locality)
 * - popFront (steal): Used by other threads to steal work (FIFO)
 * 
 * The owner pushes and pops from the back (like a stack)
 * Thieves steal from the front (oldest tasks first)
 */
template<typename T>
class WorkStealingQueue {
public:
    WorkStealingQueue() = default;
    
    // Non-copyable, non-movable (due to mutex)
    WorkStealingQueue(const WorkStealingQueue&) = delete;
    WorkStealingQueue& operator=(const WorkStealingQueue&) = delete;
    WorkStealingQueue(WorkStealingQueue&&) = delete;
    WorkStealingQueue& operator=(WorkStealingQueue&&) = delete;
    
    /**
     * Push a task to the back of the queue (owner only)
     */
    void pushBack(T item) {
        std::lock_guard<std::mutex> lock(mutex_);
        deque_.push_back(std::move(item));
        size_.fetch_add(1, std::memory_order_relaxed);
    }
    
    /**
     * Try to pop a task from the back (owner only)
     * Returns nullopt if queue is empty
     */
    std::optional<T> tryPopBack() {
        std::lock_guard<std::mutex> lock(mutex_);
        if (deque_.empty()) {
            return std::nullopt;
        }
        T item = std::move(deque_.back());
        deque_.pop_back();
        size_.fetch_sub(1, std::memory_order_relaxed);
        return item;
    }
    
    /**
     * Try to steal a task from the front (thieves)
     * Returns nullopt if queue is empty
     */
    std::optional<T> tryPopFront() {
        std::lock_guard<std::mutex> lock(mutex_);
        if (deque_.empty()) {
            return std::nullopt;
        }
        T item = std::move(deque_.front());
        deque_.pop_front();
        size_.fetch_sub(1, std::memory_order_relaxed);
        return item;
    }
    
    /**
     * Check if queue is empty (approximate)
     */
    bool empty() const {
        return size_.load(std::memory_order_relaxed) == 0;
    }
    
    /**
     * Get approximate size
     */
    size_t size() const {
        return size_.load(std::memory_order_relaxed);
    }

private:
    std::deque<T> deque_;
    mutable std::mutex mutex_;
    std::atomic<size_t> size_{0};
};

} // namespace para

#endif // WORK_STEALING_QUEUE_HPP