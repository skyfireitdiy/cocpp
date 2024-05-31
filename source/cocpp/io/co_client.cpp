#include "cocpp/io/co_client.h"
#include "cocpp/core/co_error.h"
#include "cocpp/core/co_type.h"
#include <cstddef>
#include <cstring>
#include <tuple>

CO_NAMESPACE_BEGIN

co_net co_client::get_endpoint()
{
    return endpoint__;
}

std::tuple<size_t, co_error> co_client::read(co_buffer &buf)
{
    auto ret = endpoint__.read(buf.data(), buf.size());
    if (ret < 0)
    {
        return {ret, co_error(co_error_code::co_client_read_error, "read error", strerror(errno))};
    }
    return {ret, co_error()};
}

co_error co_client::read_n(co_buffer &buf)
{
    auto remain = buf.size();
    while (remain > 0)
    {
        auto ret = endpoint__.read(buf.data() + buf.size() - remain, remain);
        if (ret < 0)
        {
            return co_error(co_error_code::co_client_read_error, "read error", strerror(errno));
        }
        remain -= ret;
    }
    return co_error();
}

std::tuple<size_t, co_error> co_client::write(const co_buffer &buf)
{
    auto ret = endpoint__.write(buf.data(), buf.size());
    if (ret < 0)
    {
        return {ret, co_error(co_error_code::co_client_write_error, "write error", strerror(errno))};
    }
    return {ret, co_error()};
}

co_error co_client::write_n(const co_buffer &buf)
{
    decltype(buf.size()) send_size = 0;
    while (send_size < buf.size())
    {
        auto ret = endpoint__.write(buf.data() + send_size, buf.size() - send_size);
        if (ret < 0)
        {
            return co_error(co_error_code::co_client_write_error, "write error", strerror(errno));
        }
        send_size += ret;
    }
    return co_error();
}

co_error co_client::close()
{
    if (endpoint__.close() < 0)
    {
        return co_error(co_error_code::co_client_close_error, "close error", strerror(errno));
    }
    return co_error();
}

CO_NAMESPACE_END
