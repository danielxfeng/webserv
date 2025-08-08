#include "Server.hpp"

// TODO:
int getTimeStamp()
{
    return 0;
}

void Server::addConn(int fd)
{
    auto now = getTimeStamp();

    t_conn conn = {fd, -1, -1, nullptr, UNKNOWN, now, now};
    conn.readBuf = std::makeshared<Buffer>();
    connMap_.emplace(fd, std::move(conn));

}

void Server::closeConn(int fd) { /* TODO: implement */ }
void Server::closeRead(int fd) { /* TODO: implement */ }

t_msg_from_serv Server::handleDataIn(int fd)
{
    t_conn conn = connMap_.at(fd);
    // TODO: CANNOT find check
    if (fd == conn.socketFd)
    {
        conn.readBuf.readFd(fd);
        if (conn.method == UNKNOWN)
        {
            // Call headerparser.
            // Assign the value to header.
        }
    }
}
t_msg_from_serv Server::handleDataOut(int fd) { /* TODO: implement */ }
void Server::timeoutKiller() { /* TODO: implement */ }