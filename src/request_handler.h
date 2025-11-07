#ifndef REQUEST_HANDLER_H
#define REQUEST_HANDLER_H

#include "cache.h"
#include "database.h"
#include <string>
#include <memory>

class RequestHandler {
public:
    RequestHandler(std::shared_ptr<LRUCache> cache, std::shared_ptr<Database> db);
    
    // Handle GET request
    std::string handle_get(const std::string& key);
    
    // Handle POST request (create)
    std::string handle_post(const std::string& key, const std::string& value);
    
    // Handle DELETE request
    std::string handle_delete(const std::string& key);
    
    // Handle stats request
    std::string handle_stats();
    
private:
    std::shared_ptr<LRUCache> cache_;
    std::shared_ptr<Database> db_;
    
    // Statistics
    uint64_t cache_hits_ = 0;
    uint64_t cache_misses_ = 0;
    uint64_t total_requests_ = 0;
};

#endif // REQUEST_HANDLER_H
