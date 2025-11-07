#ifndef THREAD_POOL_H
#define THREAD_POOL_H

#include <vector>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <memory>

class ThreadPool {
public:
    explicit ThreadPool(size_t num_threads);
    ~ThreadPool();
    
    // Submit a task to the thread pool
    void enqueue(std::function<void()> task);
    
    // Get number of threads
    size_t get_num_threads() const { return num_threads_; }
    
private:
    size_t num_threads_;
    std::vector<std::thread> workers_;
    std::queue<std::function<void()>> tasks_;
    std::mutex queue_mutex_;
    std::condition_variable condition_;
    bool stop_;
    
    void worker_thread();
};

#endif // THREAD_POOL_H
