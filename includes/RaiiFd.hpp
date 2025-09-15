#pragma once

#include "EpollHelper.hpp"
#include "LogSys.hpp"

/**
 * @brief RAII wrapper for a file descriptor.
 *
 * @details
 * - Guarantees that the fd will be released when the object goes out of scope.
 * - Ensures the fd is unregistered from epoll before being closed.
 * - Finally closes the underlying fd and resets it to -1.
 */
class RaiiFd
{
private:
    EpollHelper &epoll_helper_;
    int fd_;

    /**
     * @brief Internal cleanup routine.
     *
     * Called by both the destructor and closeFd().
     * Ensures epoll unregistration,
     * closes the underlying fd, and resets state.
     */
    void cleanUp();

public:
    RaiiFd() = delete;
    RaiiFd(const RaiiFd &o) = delete;
    RaiiFd(RaiiFd &&o) noexcept;
    RaiiFd &operator=(RaiiFd &&o) noexcept;
    RaiiFd &operator=(const RaiiFd &o) = delete;

    /**
     * @brief Constructs a RAII fd, initially invalid (fd = -1).
     */
    RaiiFd(EpollHelper &epoll_helper);

    /**
     * @brief Constructs a RAII fd with an existing descriptor.
     * @throws std::runtime_error if fd < 0
     */
    RaiiFd(EpollHelper &epoll_helper, int fd);

    /**
     * @brief Destructor of a RAII fd.
     *
     * Cleans all resources (epoll unregister, container removal, close).
     */
    ~RaiiFd();

    /**
     * @brief Returns the underlying fd.
     */
    int get() const;

    /**
     * @brief Set the fd to a given value.
     * @throws std::runtime_error if an fd already exists or if fd < 0.
     */
    void setFd(int fd);

    /**
     * @brief Check if the fd is valid (>= 0).
     */
    bool isValid() const;

    /**
     * @brief Register the fd with epoll.
     *
     * No-op if the fd is already registered.
     */
    void addToEpoll();

    /**
     * @brief Explicitly close and cleanup the fd.
     *
     * Equivalent to letting the object go out of scope, but callable manually.
     */
    void closeFd();

    bool operator==(const RaiiFd &other) const;
};

std::ostream &operator<<(std::ostream &os, const RaiiFd &r);