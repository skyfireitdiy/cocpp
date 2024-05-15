
#include "cocpp/io/co_server.h"
#include <cstring>

CO_NAMESPACE_BEGIN

co_error co_server::listen(int backlog)
{
    auto ret = endpoint__.listen(backlog);
    if (ret != 0)
    {
        return co_error(co_error_code::co_server_listen_error, "listen error", strerror(errno));
    }
    return co_error();
}

co_error co_server::close()
{
    auto ret = endpoint__.close();
    if (ret != 0)
    {
        return co_error(co_error_code::co_server_close_error, "close error", strerror(errno));
    }
    return co_error();
}

co_net co_server::get_endpoint()
{
    return endpoint__;
}

CO_NAMESPACE_END
