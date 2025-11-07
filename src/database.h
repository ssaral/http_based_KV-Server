#ifndef DATABASE_H
#define DATABASE_H

#include <string>
#include <memory>
#include <libpq-fe.h>

class Database {
public:
    Database(const std::string& connection_string);
    ~Database();
    
    // Connect to database
    bool connect();
    
    // Disconnect from database
    void disconnect();
    
    // Check if connected
    bool is_connected() const;
    
    // CRUD operations
    bool create(const std::string& key, const std::string& value);
    std::shared_ptr<std::string> read(const std::string& key);
    bool update(const std::string& key, const std::string& value);
    bool delete_key(const std::string& key);
    
private:
    std::string connection_string_;
    PGconn* conn_;
    
    bool execute_query(const std::string& query);
};

#endif // DATABASE_H
