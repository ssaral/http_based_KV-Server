#ifndef LOAD_GENERATOR_H
#define LOAD_GENERATOR_H

#include <string>
#include <vector>
#include <chrono>
#include <atomic>
#include <memory>

struct LoadGeneratorStats {
    uint64_t total_requests = 0;
    uint64_t successful_requests = 0;
    uint64_t failed_requests = 0;
    double total_response_time_ms = 0.0;
    uint64_t min_response_time_ms = UINT64_MAX;
    uint64_t max_response_time_ms = 0;
    std::chrono::system_clock::time_point start_time;
    std::chrono::system_clock::time_point end_time;
};

class LoadGenerator {
public:
    LoadGenerator(const std::string& server_url, int num_threads, int duration_seconds, const std::string& workload_type);
    
    // Run the load test
    void run();
    
    // Get statistics
    LoadGeneratorStats get_stats() const;
    
private:
    std::string server_url_;
    int num_threads_;
    int duration_seconds_;
    std::string workload_type_;
    LoadGeneratorStats stats_;
    std::atomic<bool> stop_flag_;
    std::atomic<uint64_t> total_requests_atomic_;
    std::atomic<uint64_t> successful_requests_atomic_;
    std::atomic<uint64_t> failed_requests_atomic_;
    std::atomic<double> total_response_time_atomic_;
    
    void worker_thread();
    void generate_request();
    void print_results();
};

#endif // LOAD_GENERATOR_H
