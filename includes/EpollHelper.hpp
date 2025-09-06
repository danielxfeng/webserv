#pragma once

#include <sys/epoll.h>
#include "SharedTypes.hpp"
#include "LogSys.hpp"
#include "WebServErr.hpp"

inline constexpr uint32_t FLAGS = EPOLLIN | EPOLLOUT | EPOLLRDHUP | EPOLLHUP | EPOLLERR;

/**
 * @brief RAII wrapper for epoll file descriptor management.
 */
class EpollHelper
{
private:
    int epollFd_;

public:
    /**
     * @brief Constructs an EpollHelper and creates an epoll instance.
     * @throws WebServErr::SysCallErrException if epoll_create fails.
     */
    EpollHelper();
    EpollHelper(const EpollHelper &) = delete;
    EpollHelper &operator=(const EpollHelper &) = delete;
    /**
     * @brief Destructor that cleans up the epoll instance.
     */
    ~EpollHelper();

    /**
     * @brief Cleans up the epoll instance if it is valid.
     */
    void cleanup();
    /**
     * @brief Adds a file descriptor in the epoll instance.
     * @throws WebServErr::SysCallErrException if epoll_ctl fails.
     */
    void addFd(int fd);
    /**
     * @brief Removes a file descriptor from the epoll instance.
     * @throws WebServErr::SysCallErrException if epoll_ctl fails.
     */
    void removeFd(int fd);
};