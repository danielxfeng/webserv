#include "../includes/RaiiFd.hpp"

RaiiFd::RaiiFd(EpollHelper &epoll_helper) : epoll_helper_(epoll_helper), fd_(-1) {}

RaiiFd::RaiiFd(EpollHelper &epoll_helper, int fd) : epoll_helper_(epoll_helper), fd_(fd) {}

int RaiiFd::get() const { return fd_; }
void RaiiFd::setFd(int fd)
{
    if (fd < 0)
        throw std::runtime_error("Don't set fd to a negative value, you may wanna call ~RaiiFd()");
    if (fd_ >= 0)
        throw std::runtime_error("Don't try to modify an existing fd.");
    fd_ = fd;
}

bool RaiiFd::isValid() const { return fd_ >= 0; };
void RaiiFd::addToEpoll()
{
    if (!isValid())
    {
        LOG_ERROR("Trying to add an invalid fd to epoll", "");
        return;
    }

    epoll_helper_.addFd(fd_);
    LOG_INFO("Added fd to epoll: ", fd_);
}

void RaiiFd::cleanUp()
{
    if (!isValid())
        return;

    try
    {
        epoll_helper_.removeFd(fd_);
    }
    catch (const WebServErr::SysCallErrException &)
    {
        throw;
    }
    catch (const std::exception &e)
    {
        LOG_ERROR("RaiiFd::cleanUp exception: ", e.what());
    }

    close(fd_);
    fd_ = -1;
    LOG_INFO("RaiiFd cleaned up", "");
}

void RaiiFd::closeFd()
{
    cleanUp();
}

RaiiFd::~RaiiFd()
{
    cleanUp();
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