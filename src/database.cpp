#include "database.h"
#include <iostream>
#include <sstream>

Database::Database(const std::string& connection_string)
    : connection_string_(connection_string), conn_(nullptr) {}

Database::~Database() {
    disconnect();
}

bool Database::connect() {
    conn_ = PQconnectdb(connection_string_.c_str());
    
    if (PQstatus(conn_) != CONNECTION_OK) {
        std::cerr << "Connection failed: " << PQerrorMessage(conn_) << std::endl;
        PQfinish(conn_);
        conn_ = nullptr;
        return false;
    }
    
    std::cout << "Database connection established" << std::endl;
    return true;
}

void Database::disconnect() {
    if (conn_ != nullptr) {
        PQfinish(conn_);
        conn_ = nullptr;
    }
}

bool Database::is_connected() const {
    return conn_ != nullptr && PQstatus(conn_) == CONNECTION_OK;
}

bool Database::create(const std::string& key, const std::string& value) {
    if (!is_connected()) return false;
    
    const char* paramValues[2] = {key.c_str(), value.c_str()};
    PGresult* res = PQexecParams(conn_,
        "INSERT INTO kv_store (key, value) VALUES ($1, $2) ON CONFLICT (key) DO UPDATE SET value = $2",
        2, nullptr, paramValues, nullptr, nullptr, 0);
    
    bool success = (PQresultStatus(res) == PGRES_COMMAND_OK);
    if (!success) {
        std::cerr << "Create failed: " << PQerrorMessage(conn_) << std::endl;
    }
    PQclear(res);
    return success;
}

std::shared_ptr<std::string> Database::read(const std::string& key) {
    if (!is_connected()) return nullptr;
    
    const char* paramValues[1] = {key.c_str()};
    PGresult* res = PQexecParams(conn_,
        "SELECT value FROM kv_store WHERE key = $1",
        1, nullptr, paramValues, nullptr, nullptr, 0);
    
    if (PQresultStatus(res) != PGRES_TUPLES_OK) {
        std::cerr << "Read failed: " << PQerrorMessage(conn_) << std::endl;
        PQclear(res);
        return nullptr;
    }
    
    if (PQntuples(res) == 0) {
        PQclear(res);
        return nullptr;
    }
    
    auto value = std::make_shared<std::string>(PQgetvalue(res, 0, 0));
    PQclear(res);
    return value;
}

bool Database::update(const std::string& key, const std::string& value) {
    if (!is_connected()) return false;
    
    const char* paramValues[2] = {value.c_str(), key.c_str()};
    PGresult* res = PQexecParams(conn_,
        "UPDATE kv_store SET value = $1 WHERE key = $2",
        2, nullptr, paramValues, nullptr, nullptr, 0);
    
    bool success = (PQresultStatus(res) == PGRES_COMMAND_OK);
    if (!success) {
        std::cerr << "Update failed: " << PQerrorMessage(conn_) << std::endl;
    }
    PQclear(res);
    return success;
}

bool Database::delete_key(const std::string& key) {
    if (!is_connected()) return false;
    
    const char* paramValues[1] = {key.c_str()};
    PGresult* res = PQexecParams(conn_,
        "DELETE FROM kv_store WHERE key = $1",
        1, nullptr, paramValues, nullptr, nullptr, 0);
    
    bool success = (PQresultStatus(res) == PGRES_COMMAND_OK);
    if (!success) {
        std::cerr << "Delete failed: " << PQerrorMessage(conn_) << std::endl;
    }
    PQclear(res);
    return success;
}

bool Database::execute_query(const std::string& query) {
    if (!is_connected()) return false;
    
    PGresult* res = PQexec(conn_, query.c_str());
    bool success = (PQresultStatus(res) == PGRES_COMMAND_OK);
    PQclear(res);
    return success;
}
