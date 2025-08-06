#pragma once

// TODO: to be implemented
class Server
{
private:
    unsigned int port_;

public:
    unsigned int port() const { return port_; }

    void addConn(int fd) { /* TODO: implement */ }
    void closeConn(int fd) { /* TODO: implement */ }
    bool handleRequest(int fd) { /* TODO: implement */ }
    bool sendResponse(int fd) { /* TODO: implement */ }
    void timeoutKiller() { /* TODO: implement */ }
};