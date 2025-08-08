#pragma once

#include <string>
#include <unordered_map>
#include "Buffer.hpp"

typedef enum e_direction
{
    IN,
    OUT
} t_direction;

typedef enum e_method
{
    GET,
    POST,
    DELETE,
    CGI,
    UNKNOWN,
} t_method;

typedef struct s_conn
{
    int socketFd;
    int innerFdIn;
    int innerFdOut;
    std::string path;
    t_method method;
    int startTimeStamp;
    int lastTimeStamp;
    Buffer readBuf;
    Buffer writeBuf;
} t_conn;

typedef struct s_msg_from_serv
{
    bool is_done;          // Indicates if the operation is complete
    int fd_to_register;    // File descriptor to register for epoll
    t_direction direction; // Direction of the fd_to_register
    int fd_to_unregister;  // File descriptor to unregister from epoll
} t_msg_from_serv;

// TODO: to be implemented
class Server
{
private:
    unsigned int port_;
    std::string name_;
    std::unordered_map<int, t_conn> connMap_;

public:
    unsigned int port() const { return port_; }

    void addConn(int fd) { /* TODO: implement */ }
    void closeConn(int fd) { /* TODO: implement */ }
    void closeRead(int fd) { /* TODO: implement */ }
    t_msg_from_serv handleDataIn(int fd) { /* TODO: implement */ }
    t_msg_from_serv handleDataOut(int fd) { /* TODO: implement */ }
    void timeoutKiller() { /* TODO: implement */ }
};