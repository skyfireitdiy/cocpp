#include <sys/stat.h>
#include <stdarg.h>
#include <sys/ioctl.h>
#include <string.h>

#include "cocpp/io/co_io.h"
#include "cocpp/core/co_error.h"
#include "cocpp/core/co_manager.h"
#include "cocpp/core/co_env.h"

CO_NAMESPACE_BEGIN

int co_io::init_fd(int fd)
{
    // 设置fd为非阻塞
    int flags = ::fcntl(fd, F_GETFL, 0);
    if (flags == -1)
    {
        CO_O_ERROR("fcntl error: %s", strerror(errno));
        return -1;
    }

    if (0 == (flags & O_NONBLOCK))
    {
        flags |= O_NONBLOCK;
        if (::fcntl(fd, F_SETFL, flags) == -1)
        {
            CO_O_ERROR("fcntl error: %s", strerror(errno));
            return -1;
        }
    }

    fd__ = fd;
    return 0;
}

int co_io::close()
{
    int ret = ::close(fd__);
    if (ret == -1)
    {
        CO_O_ERROR("close error: %s", strerror(errno));
        return -1;
    }
    fd__ = -1;
    return 0;
}

int co_io::read(void *buf, size_t count)
{
    for (;;)
    {
        int ret = ::read(fd__, buf, count);
        if (ret == -1)
        {
            if (errno == EAGAIN || errno == EWOULDBLOCK)
            {
                CoYield();
                continue;
            }
            else
            {
                CO_O_ERROR("read error: %s", strerror(errno));
                return -1;
            }
        }
        return ret;
    }
}

int co_io::write(const void *buf, size_t count)
{
    for (;;)
    {
        int ret = ::write(fd__, buf, count);
        if (ret == -1)
        {
            if (errno == EAGAIN || errno == EWOULDBLOCK)
            {
                CoYield();
                continue;
            }
            else
            {
                CO_O_ERROR("write error: %s", strerror(errno));
                return -1;
            }
        }
        return ret;
    }
}

int co_io::lseek(off_t offset, int whence)
{
    return ::lseek(fd__, offset, whence);
}

int co_io::fchmod(mode_t mode)
{
    return ::fchmod(fd__, mode);
}

int co_io::fchown(uid_t owner, gid_t group)
{
    return ::fchown(fd__, owner, group);
}

int co_io::fcntl(int cmd, ...)
{
    va_list args;
    va_start(args, cmd);
    int ret = ::fcntl(fd__, cmd, args);
    va_end(args);

    return ret;
}

int co_io::ioctl(int cmd, ...)
{
    va_list args;
    va_start(args, cmd);
    int ret = ::ioctl(fd__, cmd, args);
    va_end(args);
    return ret;
}

int co_io::ftruncate(off_t length)
{
    return ::ftruncate(fd__, length);
}

int co_io::fstat(struct stat *buf)
{
    return ::fstat(fd__, buf);
}

int co_io::fsync()
{
    return ::fsync(fd__);
}

int co_io::fdatasync()
{
    return ::fdatasync(fd__);
}

CO_NAMESPACE_END
