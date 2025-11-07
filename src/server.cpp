#include "server.h"
#include <httplib.h>
#include <iostream>
#include <sstream>
#include <json.hpp>

using json = nlohmann::json;

KVServer::KVServer(int port, size_t num_threads, size_t cache_size, const std::string& db_connection)
    : port_(port), num_threads_(num_threads) {
    
    thread_pool_ = std::make_shared<ThreadPool>(num_threads);
    cache_ = std::make_shared<LRUCache>(cache_size);
    db_ = std::make_shared<Database>(db_connection);
    handler_ = std::make_shared<RequestHandler>(cache_, db_);
}

bool KVServer::start() {
    // Connect to database
    if (!db_->connect()) {
        std::cerr << "Failed to connect to database" << std::endl;
        return false;
    }
    
    std::cout << "Connected to database successfully" << std::endl;
    
    httplib::Server svr;
    
    svr.Get("/api/kv", [this](const httplib::Request& req, httplib::Response& res) {
        auto key = req.get_param_value("key");
        if (key.empty()) {
            json error;
            error["error"] = "Missing key parameter";
            res.set_content(error.dump(), "application/json");
            res.status = 400;
            return;
        }
        
        try {
            std::string response = handler_->handle_get(key);
            res.set_content(response, "application/json");
            res.status = 200;
        } catch (const std::exception& e) {
            json error;
            error["error"] = e.what();
            res.set_content(error.dump(), "application/json");
            res.status = 500;
        }
    });
    
    svr.Post("/api/kv", [this](const httplib::Request& req, httplib::Response& res) {
        try {
            json body = json::parse(req.body);
            
            if (!body.contains("key") || !body.contains("value")) {
                json error;
                error["error"] = "Missing key or value in request body";
                res.set_content(error.dump(), "application/json");
                res.status = 400;
                return;
            }
            
            std::string key = body["key"].get<std::string>();
            std::string value = body["value"].get<std::string>();
            
            std::string response = handler_->handle_post(key, value);
            res.set_content(response, "application/json");
            res.status = 200;
        } catch (const json::exception& e) {
            json error;
            error["error"] = "Invalid JSON in request body";
            res.set_content(error.dump(), "application/json");
            res.status = 400;
        } catch (const std::exception& e) {
            json error;
            error["error"] = e.what();
            res.set_content(error.dump(), "application/json");
            res.status = 500;
        }
    });
    
    svr.Delete("/api/kv", [this](const httplib::Request& req, httplib::Response& res) {
        auto key = req.get_param_value("key");
        if (key.empty()) {
            json error;
            error["error"] = "Missing key parameter";
            res.set_content(error.dump(), "application/json");
            res.status = 400;
            return;
        }
        
        try {
            std::string response = handler_->handle_delete(key);
            res.set_content(response, "application/json");
            res.status = 200;
        } catch (const std::exception& e) {
            json error;
            error["error"] = e.what();
            res.set_content(error.dump(), "application/json");
            res.status = 500;
        }
    });
    
    svr.Get("/api/stats", [this](const httplib::Request& req, httplib::Response& res) {
        try {
            std::string response = handler_->handle_stats();
            res.set_content(response, "application/json");
            res.status = 200;
        } catch (const std::exception& e) {
            json error;
            error["error"] = e.what();
            res.set_content(error.dump(), "application/json");
            res.status = 500;
        }
    });
    
    svr.Get("/health", [](const httplib::Request& req, httplib::Response& res) {
        json health;
        health["status"] = "healthy";
        res.set_content(health.dump(), "application/json");
        res.status = 200;
    });
    
    svr.set_post_routing_handler([this](const httplib::Request& req, httplib::Response& res) {
        // This allows us to use the thread pool for request handling
    });
    
    std::cout << "Starting KV Server on port " << port_ << " with " << num_threads_ << " threads" << std::endl;
    std::cout << "Cache size: " << cache_->get_max_size() << " entries" << std::endl;
    std::cout << "Server ready. Listening on http://0.0.0.0:" << port_ << std::endl;
    
    // Start listening (blocking call)
    if (!svr.listen("0.0.0.0", port_)) {
        std::cerr << "Failed to start server on port " << port_ << std::endl;
        return false;
    }
    
    return true;
}

void KVServer::stop() {
    std::cout << "Stopping KV Server..." << std::endl;
}

int main(int argc, char* argv[]) {
    int port = 8080;
    size_t num_threads = 4;
    size_t cache_size = 1000;
    std::string db_connection = "host=localhost user=postgres password=postgres dbname=kvstore";
    
    // Parse command line arguments
    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        if (arg == "--port" && i + 1 < argc) {
            port = std::stoi(argv[++i]);
        } else if (arg == "--threads" && i + 1 < argc) {
            num_threads = std::stoi(argv[++i]);
        } else if (arg == "--cache-size" && i + 1 < argc) {
            cache_size = std::stoi(argv[++i]);
        } else if (arg == "--db-conn" && i + 1 < argc) {
            db_connection = argv[++i];
        } else if (arg == "--help") {
            std::cout << "Usage: kv_server [options]\n"
                      << "Options:\n"
                      << "  --port <port>              Server port (default: 8080)\n"
                      << "  --threads <num>            Number of worker threads (default: 4)\n"
                      << "  --cache-size <size>        Cache size in entries (default: 1000)\n"
                      << "  --db-conn <connection>     PostgreSQL connection string\n"
                      << "  --help                     Show this help message\n";
            return 0;
        }
    }
    
    KVServer server(port, num_threads, cache_size, db_connection);
    
    if (!server.start()) {
        std::cerr << "Failed to start server" << std::endl;
        return 1;
    }
    
    return 0;
}
