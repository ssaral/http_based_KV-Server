#include "request_handler.h"
#include <json.hpp>
#include <mutex>

using json = nlohmann::json;

static std::mutex stats_mutex_;

RequestHandler::RequestHandler(std::shared_ptr<LRUCache> cache, std::shared_ptr<Database> db)
    : cache_(cache), db_(db) {}

std::string RequestHandler::handle_get(const std::string& key) {
    std::unique_lock<std::mutex> lock(stats_mutex_);
    total_requests_++;
    lock.unlock();
    
    // Try cache first
    auto cached_value = cache_->get(key);
    if (cached_value) {
        lock.lock();
        cache_hits_++;
        lock.unlock();
        
        json response;
        response["key"] = key;
        response["value"] = *cached_value;
        response["source"] = "cache";
        return response.dump();
    }
    
    // Cache miss - fetch from database
    lock.lock();
    cache_misses_++;
    lock.unlock();
    
    auto db_value = db_->read(key);
    if (!db_value) {
        json error;
        error["error"] = "Key not found";
        return error.dump();
    }
    
    // Put in cache for future access
    cache_->put(key, *db_value);
    
    json response;
    response["key"] = key;
    response["value"] = *db_value;
    response["source"] = "database";
    return response.dump();
}

std::string RequestHandler::handle_post(const std::string& key, const std::string& value) {
    std::unique_lock<std::mutex> lock(stats_mutex_);
    total_requests_++;
    lock.unlock();
    
    // Store in both cache and database
    cache_->put(key, value);
    bool db_success = db_->create(key, value);
    
    json response;
    if (db_success) {
        response["status"] = "success";
        response["key"] = key;
    } else {
        response["status"] = "error";
        response["message"] = "Failed to create in database";
    }
    return response.dump();
}

std::string RequestHandler::handle_delete(const std::string& key) {
    std::unique_lock<std::mutex> lock(stats_mutex_);
    total_requests_++;
    lock.unlock();
    
    // Delete from database first
    bool db_success = db_->delete_key(key);
    
    // Delete from cache
    cache_->remove(key);
    
    json response;
    if (db_success) {
        response["status"] = "success";
        response["key"] = key;
    } else {
        response["status"] = "error";
        response["message"] = "Failed to delete from database";
    }
    return response.dump();
}

std::string RequestHandler::handle_stats() {
    std::unique_lock<std::mutex> lock(stats_mutex_);
    
    json stats;
    stats["cache_hits"] = cache_hits_;
    stats["cache_misses"] = cache_misses_;
    stats["total_requests"] = total_requests_;
    stats["cache_size"] = cache_->get_size();
    stats["cache_max_size"] = cache_->get_max_size();
    
    stats["cache_evictions"] = cache_->get_evictions();
    
    if (total_requests_ > 0) {
        stats["hit_rate"] = (double)cache_hits_ / total_requests_;
    }
    
    return stats.dump();
}
