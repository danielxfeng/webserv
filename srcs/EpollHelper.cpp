#include "../includes/EpollHelper.hpp"

EpollHelper::EpollHelper()
{
    epollFd_ = epoll_create(1);
    if (epollFd_ == -1)
        throw WebServErr::SysCallErrException("epoll_create failed");
}

EpollHelper::EpollHelper(EpollHelper&& other) noexcept
    : epollFd_(other.epollFd_)
{
    other.epollFd_ = -1;
}

EpollHelper& EpollHelper::operator=(EpollHelper&& other) noexcept
{
    if (this != &other)
    {
        cleanup();
        epollFd_ = other.epollFd_;
        other.epollFd_ = -1;
    }
    return *this;
}

EpollHelper::~EpollHelper() { cleanup(); }

void EpollHelper::cleanup()
{
    if (epollFd_ < 0)
        return;
    close(epollFd_);
    epollFd_ = -1;
}

int EpollHelper::getEpollFd() const { return epollFd_; }

void EpollHelper::addFd(int fd)
{
    if (fd < 0)
    {
        return;
    }

    struct epoll_event event{};
    event.events = FLAGS;
    event.data.fd = fd;

    if (epoll_ctl(epollFd_, EPOLL_CTL_MOD, fd, &event) == -1)
    {
        if (errno == ENOENT)
        {
            if (epoll_ctl(epollFd_, EPOLL_CTL_ADD, fd, &event) == -1)
                throw WebServErr::SysCallErrException("epoll_ctl ADD failed");
        }
        else
            throw WebServErr::SysCallErrException("epoll_ctl MOD failed");
    }
}

void EpollHelper::removeFd(int fd)
{
    if (fd < 0)
    {
        return;
    }
    if (epoll_ctl(epollFd_, EPOLL_CTL_DEL, fd, nullptr) == -1 && errno != ENOENT)
        throw WebServErr::SysCallErrException("epoll_ctl failed");
}
