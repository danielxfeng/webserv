#include "../includes/RaiiFd.hpp"

RaiiFd::RaiiFd(EpollHelper &epoll_helper) : epoll_helper_(epoll_helper), fd_(-1) {}

RaiiFd::RaiiFd(EpollHelper &epoll_helper, int fd) : epoll_helper_(epoll_helper), fd_(fd) {}

RaiiFd::RaiiFd(RaiiFd&& other) noexcept 
    : epoll_helper_(other.epoll_helper_), fd_(other.fd_) 
{
    other.fd_ = -1; // invalidate the moved-from object
}

RaiiFd& RaiiFd::operator=(RaiiFd&& other) noexcept 
{
    if (this != &other) {
        cleanUp();
        epoll_helper_ = std::move(other.epoll_helper_);
        fd_ = other.fd_;
        other.fd_ = -1; // invalidate
    }
    return *this;
}

int RaiiFd::get() const { return fd_; }
void RaiiFd::setFd(int fd)
{
    if (fd < 0)
        throw WebServErr::SysCallErrException("Invalid fd value.");
    if (fd_ >= 0)
        throw std::runtime_error("Don't try to modify an existing fd.");
    fd_ = fd;
}

bool RaiiFd::isValid() const { return fd_ >= 0; };
void RaiiFd::addToEpoll()
{
    if (!isValid())
    {
        return;
    }

    epoll_helper_.addFd(fd_);
}

void RaiiFd::cleanUp()
{
    if (!isValid())
        return;

    try
    {
        epoll_helper_.removeFd(fd_);
    }
    catch (const WebServErr::SysCallErrException &e)
    {
    }
    catch (const std::exception &e)
    {
    }
    
    close(fd_);
    fd_ = -1;
}

void RaiiFd::closeFd()
{
    cleanUp();
}

RaiiFd::~RaiiFd()
{
    try
    {
        cleanUp();
    }
    catch (const std::exception &e)
    {
    }
}

bool RaiiFd::operator==(const RaiiFd &o) const
{
    if (!isValid() || !o.isValid())
        return false;
    return fd_ == o.fd_;
}

std::ostream &operator<<(std::ostream &os, const RaiiFd &r)
{
    os << "RaiiFd{" << r.get() << "}";
    return os;
}