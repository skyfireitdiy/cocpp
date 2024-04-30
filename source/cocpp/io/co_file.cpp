#include "cocpp/io/co_file.h"

CO_NAMESPACE_BEGIN

int co_file::open(const char *pathname, int flags, mode_t mode)
{
    fd__ = ::open(pathname, flags | O_NONBLOCK, mode);

    if (fd__ == -1)
    {
        return -1;
    }

    return 0;
}

CO_NAMESPACE_END
