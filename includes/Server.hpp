#pragma once

typedef enum
{
    IN,
    OUT
} t_direction;

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

public:
    unsigned int port() const { return port_; }

    void addConn(int fd) { /* TODO: implement */ }
    void closeConn(int fd) { /* TODO: implement */ }
    void closeRead(int fd) { /* TODO: implement */ }
    t_msg_from_serv handleRequest(int fd) { /* TODO: implement */ }
    t_msg_from_serv sendResponse(int fd) { /* TODO: implement */ }
    void timeoutKiller() { /* TODO: implement */ }
};