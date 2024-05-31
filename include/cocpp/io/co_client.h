
_Pragma("once");
#include <vector>
#include "cocpp/core/co_type.h"
#include "cocpp/core/co_error.h"
#include "cocpp/core/co_define.h"
#include "cocpp/io/co_net.h"

CO_NAMESPACE_BEGIN

class co_client
{
protected:
    co_net endpoint__;

public:
    co_client() = default;
    virtual ~co_client() = default;

    virtual std::tuple<size_t, co_error> read(co_buffer &buffer) = 0;
    virtual co_error read_n(co_buffer &buffer) = 0;
    virtual std::tuple<size_t, co_error> write(const co_buffer &buffer) = 0;
    virtual co_error write_n(const co_buffer &buffer) = 0;
    virtual co_error close() = 0;
    virtual co_net get_endpoint() = 0;
};

CO_NAMESPACE_END
