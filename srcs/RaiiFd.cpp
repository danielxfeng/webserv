#include "../includes/RaiiFd.hpp"

RaiiFd::RaiiFd(WebServ &web_serv, Server &server) : web_serv_(web_serv), server_(server), fd_(-1), is_in_epoll_in_(false), is_in_epoll_out_(false) {}

RaiiFd::RaiiFd(WebServ &web_serv, Server &server, int fd) : web_serv_(web_serv), server_(server), fd_(fd), is_in_epoll_in_(false), is_in_epoll_out_(false) {}

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
void RaiiFd::addToEpoll(t_direction direction)
{
    if (!isValid())
    {
        LOG_ERROR("Trying to add an invalid fd to epoll", "");
        return;
    }

    if ((direction == IN && is_in_epoll_in_) || (direction == OUT && is_in_epoll_out_))
    {
        LOG_DEBUG("fd already in epoll for this direction, skipping", "");
        return;
    }
    web_serv_.addConn(fd_, server_, direction);
    if (direction == IN)
        is_in_epoll_in_ = true;
    else
        is_in_epoll_out_ = true;
}

void RaiiFd::cleanUp()
{
    if (!isValid())
        return;

    try
    {
        web_serv_.closeConn(fd_);
        server_.closeConn(fd_);
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
    is_in_epoll_in_ = false;
    is_in_epoll_out_ = false;
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