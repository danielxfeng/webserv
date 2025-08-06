#pragma once

// TODO: to be implemented
class Server
{
private:
    unsigned int port_;

public:
    unsigned int port() const { return port_; }

    void addConn(int fd) { (void)fd; }
    void closeConn(int fd) { (void)fd; }
    bool handleRequest(int fd) { (void)fd; return false; }
    bool sendResponse(int fd) { (void)fd; return false; }
    void timeoutKiller() { }
};