#include "cache.h"

LRUCache::LRUCache(size_t max_size) : max_size_(max_size) {}

std::shared_ptr<std::string> LRUCache::get(const std::string& key) {
    std::unique_lock<std::mutex> lock(cache_mutex_);
    
    auto it = cache_map_.find(key);
    if (it == cache_map_.end()) {
        misses_++;
        return nullptr;  // Cache miss
    }
    
    // Move to back (most recently used)
    auto entry_it = it->second;
    lru_list_.splice(lru_list_.end(), lru_list_, entry_it);
    hits_++;
    
    return std::make_shared<std::string>(entry_it->value);
}

void LRUCache::put(const std::string& key, const std::string& value) {
    std::unique_lock<std::mutex> lock(cache_mutex_);
    
    auto it = cache_map_.find(key);
    if (it != cache_map_.end()) {
        // Update existing entry
        it->second->value = value;
        lru_list_.splice(lru_list_.end(), lru_list_, it->second);
        return;
    }
    
    // Evict if cache is full
    if (lru_list_.size() >= max_size_) {
        evict_lru();
    }
    
    // Add new entry to back (most recently used)
    lru_list_.push_back({key, value});
    cache_map_[key] = std::prev(lru_list_.end());
}

bool LRUCache::remove(const std::string& key) {
    std::unique_lock<std::mutex> lock(cache_mutex_);
    
    auto it = cache_map_.find(key);
    if (it == cache_map_.end()) {
        return false;
    }
    
    lru_list_.erase(it->second);
    cache_map_.erase(it);
    return true;
}

bool LRUCache::exists(const std::string& key) {
    std::unique_lock<std::mutex> lock(cache_mutex_);
    return cache_map_.find(key) != cache_map_.end();
}

size_t LRUCache::get_size() const {
    std::unique_lock<std::mutex> lock(cache_mutex_);
    return lru_list_.size();
}

uint64_t LRUCache::get_hits() const {
    std::unique_lock<std::mutex> lock(cache_mutex_);
    return hits_;
}

uint64_t LRUCache::get_misses() const {
    std::unique_lock<std::mutex> lock(cache_mutex_);
    return misses_;
}

uint64_t LRUCache::get_evictions() const {
    std::unique_lock<std::mutex> lock(cache_mutex_);
    return evictions_;
}

void LRUCache::reset_stats() {
    std::unique_lock<std::mutex> lock(cache_mutex_);
    hits_ = 0;
    misses_ = 0;
    evictions_ = 0;
}

void LRUCache::evict_lru() {
    if (!lru_list_.empty()) {
        auto& front_entry = lru_list_.front();
        cache_map_.erase(front_entry.key);
        lru_list_.pop_front();
        evictions_++;
    }
}
