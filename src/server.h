#ifndef SERVER_H
#define SERVER_H

#include "thread_pool.h"
#include "cache.h"
#include "database.h"
#include "request_handler.h"
#include <memory>
#include <string>

class KVServer {
public:
    KVServer(int port, size_t num_threads, size_t cache_size, const std::string& db_connection);
    
    // Start the server
    bool start();
    
    // Stop the server
    void stop();
    
private:
    int port_;
    size_t num_threads_;
    std::shared_ptr<ThreadPool> thread_pool_;
    std::shared_ptr<LRUCache> cache_;
    std::shared_ptr<Database> db_;
    std::shared_ptr<RequestHandler> handler_;
};

#endif // SERVER_H
