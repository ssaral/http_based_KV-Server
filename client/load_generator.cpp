#include "load_generator.h"
#include <httplib.h>
#include <thread>
#include <iostream>
#include <random>
#include <iomanip>
#include <cmath>
#include <json.hpp>

using json = nlohmann::json;

LoadGenerator::LoadGenerator(const std::string& server_url, int num_threads, int duration_seconds, const std::string& workload_type)
    : server_url_(server_url), num_threads_(num_threads), duration_seconds_(duration_seconds), 
      workload_type_(workload_type), stop_flag_(false),
      total_requests_atomic_(0), successful_requests_atomic_(0), failed_requests_atomic_(0),
      total_response_time_atomic_(0.0) {}

void LoadGenerator::run() {
    stats_.start_time = std::chrono::system_clock::now();
    
    std::cout << "Starting load test..." << std::endl;
    std::cout << "Server URL: " << server_url_ << std::endl;
    std::cout << "Number of threads: " << num_threads_ << std::endl;
    std::cout << "Duration: " << duration_seconds_ << " seconds" << std::endl;
    std::cout << "Workload type: " << workload_type_ << std::endl;
    std::cout << std::string(50, '-') << std::endl;
    
    std::vector<std::thread> threads;
    for (int i = 0; i < num_threads_; ++i) {
        threads.emplace_back([this] { worker_thread(); });
    }
    
    // Wait for duration
    std::this_thread::sleep_for(std::chrono::seconds(duration_seconds_));
    stop_flag_ = true;
    
    // Wait for all threads to finish
    for (auto& t : threads) {
        if (t.joinable()) {
            t.join();
        }
    }
    
    stats_.end_time = std::chrono::system_clock::now();
    stats_.total_requests = total_requests_atomic_.load();
    stats_.successful_requests = successful_requests_atomic_.load();
    stats_.failed_requests = failed_requests_atomic_.load();
    stats_.total_response_time_ms = total_response_time_atomic_.load();
    
    print_results();
}

void LoadGenerator::worker_thread() {
    std::random_device rd;
    std::mt19937 gen(rd() + std::hash<std::thread::id>{}(std::this_thread::get_id())); //std::this_thread::get_id());
    
    while (!stop_flag_) {
        generate_request();
    }
}

void LoadGenerator::generate_request() {
    std::random_device rd;
    std::mt19937 gen(rd() + std::hash<std::thread::id>{}(std::this_thread::get_id())); //std::this_thread::get_id());
    std::uniform_int_distribution<> key_dist(1, 100000);
    std::uniform_real_distribution<> op_dist(0.0, 1.0);
    
    httplib::Client cli(server_url_);
    cli.set_connection_timeout(0, 500000);  // 500ms timeout
    cli.set_read_timeout(1, 0);
    
    auto start_time = std::chrono::high_resolution_clock::now();
    
    try {
        std::string key = "key:" + std::to_string(key_dist(gen));
        std::string value = "value:" + std::to_string(key_dist(gen));
        
        bool success = false;
        
        if (workload_type_ == "put_all") {
            // Create/Delete only
            double op = op_dist(gen);
            if (op < 0.5) {
                json body;
                body["key"] = key;
                body["value"] = value;
                auto res = cli.Post("/api/kv", body.dump(), "application/json");
                success = (res && res->status == 200);
            } else {
                auto res = cli.Delete("/api/kv?key=" + key);
                success = (res && res->status == 200);
            }
        } 
        else if (workload_type_ == "get_all") {
            // Read unique keys only
            auto res = cli.Get("/api/kv?key=" + key);
            success = (res && (res->status == 200 || res->status == 404));
        } 
        else if (workload_type_ == "get_popular") {
            // Read same keys repeatedly (popular keys)
            std::uniform_int_distribution<> popular_dist(1, 100);  // Only 100 popular keys
            std::string popular_key = "popular:" + std::to_string(popular_dist(gen));
            auto res = cli.Get("/api/kv?key=" + popular_key);
            success = (res && (res->status == 200 || res->status == 404));
        } 
        else if (workload_type_ == "get_put") {
            // Mixed workload
            double op = op_dist(gen);
            if (op < 0.7) {
                // 70% reads
                auto res = cli.Get("/api/kv?key=" + key);
                success = (res && (res->status == 200 || res->status == 404));
            } else {
                // 30% writes
                json body;
                body["key"] = key;
                body["value"] = value;
                auto res = cli.Post("/api/kv", body.dump(), "application/json");
                success = (res && res->status == 200);
            }
        } 
        else {
            // Default: get_all
            auto res = cli.Get("/api/kv?key=" + key);
            success = (res && (res->status == 200 || res->status == 404));
        }
        
        auto end_time = std::chrono::high_resolution_clock::now();
        auto response_time = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time).count();
        
        total_requests_atomic_++;
        total_response_time_atomic_ = total_response_time_atomic_ + response_time;
        
        if (success) {
            successful_requests_atomic_++;
        } else {
            failed_requests_atomic_++;
        }
        
    } catch (const std::exception& e) {
        total_requests_atomic_++;
        failed_requests_atomic_++;
    }
}

LoadGeneratorStats LoadGenerator::get_stats() const {
    return stats_;
}

void LoadGenerator::print_results() {
    auto duration = std::chrono::duration_cast<std::chrono::seconds>(stats_.end_time - stats_.start_time);
    double throughput = (double)stats_.successful_requests / duration.count();
    double avg_response_time = stats_.total_response_time_ms / std::max(1UL, stats_.successful_requests);
    
    std::cout << std::string(50, '-') << std::endl;
    std::cout << "\n=== Load Test Results ===" << std::endl;
    std::cout << "Workload Type: " << workload_type_ << std::endl;
    std::cout << "Duration: " << duration.count() << " seconds" << std::endl;
    std::cout << "Total Requests: " << stats_.total_requests << std::endl;
    std::cout << "Successful Requests: " << stats_.successful_requests << std::endl;
    std::cout << "Failed Requests: " << stats_.failed_requests << std::endl;
    std::cout << "Throughput: " << std::fixed << std::setprecision(2) << throughput << " req/sec" << std::endl;
    std::cout << "Avg Response Time: " << std::fixed << std::setprecision(2) << avg_response_time << " ms" << std::endl;
    std::cout << "Success Rate: " << std::fixed << std::setprecision(2) 
              << (100.0 * stats_.successful_requests / stats_.total_requests) << "%" << std::endl;
    std::cout << std::endl;
}

int main(int argc, char* argv[]) {
    std::string server_url = "http://localhost:8080";
    int num_threads = 10;
    int duration = 60;
    std::string workload_type = "get_all";
    
    // Parse command line arguments
    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        if (arg == "--url" && i + 1 < argc) {
            server_url = argv[++i];
        } else if (arg == "--threads" && i + 1 < argc) {
            num_threads = std::stoi(argv[++i]);
        } else if (arg == "--duration" && i + 1 < argc) {
            duration = std::stoi(argv[++i]);
        } else if (arg == "--workload" && i + 1 < argc) {
            workload_type = argv[++i];
        } else if (arg == "--help") {
            std::cout << "Usage: load_generator [options]\n"
                      << "Options:\n"
                      << "  --url <url>              Server URL (default: http://localhost:8080)\n"
                      << "  --threads <num>          Number of client threads (default: 10)\n"
                      << "  --duration <seconds>     Test duration in seconds (default: 60)\n"
                      << "  --workload <type>        Workload type: put_all, get_all, get_popular, get_put\n"
                      << "  --help                   Show this help message\n";
            return 0;
        }
    }
    
    LoadGenerator generator(server_url, num_threads, duration, workload_type);
    generator.run();
    
    return 0;
}
