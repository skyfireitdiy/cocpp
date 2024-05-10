#include "cocpp/io/co_file.h"

CO_NAMESPACE_BEGIN

std::optional<co_file> co_file::open(const char *pathname, int flags, mode_t mode)
{
    auto fd = ::open(pathname, flags | O_NONBLOCK, mode);

    if (fd == -1)
    {
        return std::nullopt;
    }

    co_file file;
    file.init_fd(fd);
    return file;
}

CO_NAMESPACE_END
