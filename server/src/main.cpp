#include "httplib.h"
#include <iostream>
#include <pqxx/pqxx>   // C++ wrapper for libpq
#include <cstdlib>

int main() {
    const char* port_env = std::getenv("SERVER_PORT");
    const char* db_host = std::getenv("DB_HOST");
    const char* db_user = std::getenv("POSTGRES_USER");
    const char* db_pass = std::getenv("POSTGRES_PASSWORD");
    const char* db_name = std::getenv("POSTGRES_DB");

    int port = port_env ? std::stoi(port_env) : 8080;

    std::cout << "Server starting on port " << port << std::endl;
    std::cout << "Connecting to DB at " << (db_host ? db_host : "localhost") << std::endl;

    try {
        std::string conn_str =
            "host=" + std::string(db_host ? db_host : "localhost") +
            " user=" + std::string(db_user ? db_user : "postgres") +
            " password=" + std::string(db_pass ? db_pass : "postgres") +
            " dbname=" + std::string(db_name ? db_name : "kvstore");

        pqxx::connection C(conn_str);
        if (C.is_open())
            std::cout << "DB connection successful \n";
        else
            std::cout << "DB connection failed \n";
    } catch (const std::exception &e) {
        std::cerr << "DB error: " << e.what() << std::endl;
    }

    httplib::Server svr;
    svr.Get("/", [](const httplib::Request&, httplib::Response &res) {
        res.set_content("Hello World from C++ HTTP server!", "text/plain");
    });

    svr.listen("0.0.0.0", port);
    return 0;
}
