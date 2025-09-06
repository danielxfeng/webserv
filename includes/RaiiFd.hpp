#pragma once

#include "WebServ.hpp"
#include "LogSys.hpp"

/**
 * @brief RAII wrapper for a file descriptor.
 *
 * @details
 * - Guarantees that the fd will be released when the object goes out of scope.
 * - Ensures the fd is unregistered from epoll before being closed.
 * - Notifies WebServ and Server to unregister the fd from their containers.
 * - Finally closes the underlying fd and resets it to -1.
 */
class RaiiFd
{
private:
    WebServ &web_serv_;
    Server &server_;
    int fd_;
    bool is_in_epoll_in_;
    bool is_in_epoll_out_;

    /**
     * @brief Internal cleanup routine.
     *
     * Called by both the destructor and closeFd().
     * Ensures epoll unregistration, container cleanup,
     * closes the underlying fd, and resets state.
     */
    void cleanUp();

public:
    RaiiFd() = delete;
    RaiiFd(const RaiiFd &o) = delete;
    RaiiFd &operator=(const RaiiFd &o) = delete;

    /**
     * @brief Constructs a RAII fd, initially invalid (fd = -1).
     */
    RaiiFd(WebServ &web_serv, Server &server);

    /**
     * @brief Constructs a RAII fd with an existing descriptor.
     * @throws std::runtime_error if fd < 0
     */
    RaiiFd(WebServ &web_serv, Server &server, int fd);

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
     * @brief Register the fd with epoll for the given direction.
     *
     * No-op if the fd is already registered for the same direction.
     */
    void addToEpoll(t_direction direction);

    /**
     * @brief Explicitly close and cleanup the fd.
     *
     * Equivalent to letting the object go out of scope, but callable manually.
     */
    void closeFd();

    bool operator==(const RaiiFd &other) const;
};

std::ostream &operator<<(std::ostream &os, const RaiiFd &r);