#ifndef CACHE_H
#define CACHE_H

#include <unordered_map>
#include <list>
#include <mutex>
#include <string>
#include <memory>
#include <cstdint>

struct CacheEntry {
    std::string key;
    std::string value;
};

class LRUCache {
public:
    explicit LRUCache(size_t max_size);
    
    // Get value from cache (returns nullptr if not found)
    std::shared_ptr<std::string> get(const std::string& key);
    
    // Put key-value pair in cache
    void put(const std::string& key, const std::string& value);
    
    // Delete key from cache
    bool remove(const std::string& key);
    
    // Check if key exists
    bool exists(const std::string& key);
    
    // Get cache statistics
    size_t get_size() const;
    size_t get_max_size() const { return max_size_; }
    
    uint64_t get_hits() const;
    uint64_t get_misses() const;
    uint64_t get_evictions() const;
    void reset_stats();
    
private:
    size_t max_size_;
    std::unordered_map<std::string, std::list<CacheEntry>::iterator> cache_map_;
    std::list<CacheEntry> lru_list_;  // Most recent at back, least recent at front
    mutable std::mutex cache_mutex_;
    
    uint64_t hits_ = 0;
    uint64_t misses_ = 0;
    uint64_t evictions_ = 0;
    
    void evict_lru();
};

#endif // CACHE_H
