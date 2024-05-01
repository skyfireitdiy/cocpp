#include <sys/socket.h>
#include <string.h>
#include "cocpp/io/co_net.h"
#include "cocpp/core/co_define.h"
#include "cocpp/core/co_manager.h"
#include "cocpp/core/co_env.h"

CO_NAMESPACE_BEGIN

int co_net::socket(int domain, int type, int protocol)
{
    fd__ = ::socket(domain, type, protocol);

    if (fd__ == -1)
    {
        CO_O_ERROR("socket error: %s", strerror(errno));
        return -1;
    }

    int flags = ::fcntl(fd__, F_GETFL, 0);
    if (flags == -1)
    {
        CO_O_ERROR("fcntl error: %s", strerror(errno));
        ::close(fd__);
        fd__ = -1;
        return -1;
    }

    if (0 == (flags & O_NONBLOCK))
    {
        flags |= O_NONBLOCK;
        if (::fcntl(fd__, F_SETFL, flags) == -1)
        {
            CO_O_ERROR("fcntl error: %s", strerror(errno));
            ::close(fd__);
            fd__ = -1;
            return -1;
        }
    }

    return 0;
}

int co_net::listen(int backlog)
{
    return ::listen(fd__, backlog);
}

int co_net::accept(struct sockaddr *addr, socklen_t *addrlen)
{
    for (;;)
    {
        int fd = ::accept(fd__, addr, addrlen);

        if (fd == -1)
        {
            if (errno == EINTR || errno == EAGAIN || errno == EWOULDBLOCK)
            {
                CoYield();
                continue;
            }
            CO_O_ERROR("accept error: %s", strerror(errno));
            return -1;
        }
        return fd;
    }
}

int co_net::bind(const struct sockaddr *addr, socklen_t addrlen)
{
    return ::bind(fd__, addr, addrlen);
}

int co_net::connect(const struct sockaddr *addr, socklen_t addrlen)
{
    int ret = ::connect(fd__, addr, addrlen);

    if (ret > 0)
    {
        return ret;
    }

    if (errno != EINPROGRESS)
    {
        CO_O_ERROR("connect error: %s", strerror(errno));
        return -1;
    }

    fd_set write_fds;
    struct timeval timeout;
    timeout.tv_sec = 0;
    timeout.tv_usec = 10 * 1000;
    while (1)
    {
        FD_ZERO(&write_fds);
        FD_SET(fd__, &write_fds);

        int result = select(fd__ + 1, NULL, &write_fds, NULL, &timeout);

        if (result < 0)
        {
            CO_O_ERROR("select error: %s", strerror(errno));
            return -1;
        }
        else if (result == 0)
        {
            CoYield();
            continue;
        }
        else
        {
            if (FD_ISSET(fd__, &write_fds))
            {
                return 0;
            }
        }
    }
}

int co_net::send(const void *buf, size_t len, int flags)
{
    for (;;)
    {
        int ret = ::send(fd__, buf, len, flags);

        if (ret == -1)
        {
            if (errno == EINTR || errno == EAGAIN || errno == EWOULDBLOCK)
            {
                CoYield();
                continue;
            }
            CO_O_ERROR("send error: %s", strerror(errno));
            return -1;
        }
        return ret;
    }
}

int co_net::sendto(const void *buf, size_t len, int flags, const struct sockaddr *dest_addr, socklen_t addrlen)
{
    return ::sendto(fd__, buf, len, flags, dest_addr, addrlen);
}

int co_net::recv(void *buf, size_t len, int flags)
{
    for (;;)
    {
        int ret = ::recv(fd__, buf, len, flags);

        if (ret == -1)
        {
            if (errno == EINTR || errno == EAGAIN || errno == EWOULDBLOCK)
            {
                CoYield();
                continue;
            }
            CO_O_ERROR("recv error: %s", strerror(errno));
            return -1;
        }
        return ret;
    }
}

int co_net::recvfrom(void *buf, size_t len, int flags, struct sockaddr *src_addr, socklen_t *addrlen)
{
    for (;;)
    {
        int ret = ::recvfrom(fd__, buf, len, flags, src_addr, addrlen);

        if (ret == -1)
        {
            if (errno == EINTR || errno == EAGAIN || errno == EWOULDBLOCK)
            {
                CoYield();
                continue;
            }
            CO_O_ERROR("recvfrom error: %s", strerror(errno));
            return -1;
        }
        return ret;
    }
}

int co_net::getsockopt(int level, int optname, void *optval, socklen_t *optlen)
{
    return ::getsockopt(fd__, level, optname, optval, optlen);
}

int co_net::setsockopt(int level, int optname, const void *optval, socklen_t optlen)
{
    return ::setsockopt(fd__, level, optname, optval, optlen);
}

int co_net::shutdown(int how)
{
    return ::shutdown(fd__, how);
}

int co_net::sendmsg(const struct msghdr *msg, int flags)
{
    for (;;)
    {
        int ret = ::sendmsg(fd__, msg, flags);
        if (ret == -1)
        {
            if (errno == EINTR || errno == EAGAIN || errno == EWOULDBLOCK)
            {
                CoYield();
                continue;
            }
            CO_O_ERROR("sendmsg error: %s", strerror(errno));
            return -1;
        }
        return ret;
    }
}

int co_net::recvmsg(struct msghdr *msg, int flags)
{
    for (;;)
    {
        int ret = ::recvmsg(fd__, msg, flags);
        if (ret == -1)
        {
            if (errno == EINTR || errno == EAGAIN || errno == EWOULDBLOCK)
            {
                CoYield();
                continue;
            }
            CO_O_ERROR("recvmsg error: %s", strerror(errno));
            return -1;
        }
        return ret;
    }
}

CO_NAMESPACE_END
