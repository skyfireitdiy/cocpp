_Pragma("once");
#include <memory>
#include "cocpp/core/co_error.h"
#include "cocpp/io/co_net.h"
#include "cocpp/core/co_define.h"

CO_NAMESPACE_BEGIN

class co_client;

class co_server
{
protected:
    co_net endpoint__;

public:
    co_server() = default;

    virtual co_error listen(int backlog) = 0;
    virtual std::tuple<std::shared_ptr<co_client>, co_error> accept();
    virtual co_error close();

    virtual ~co_server() = default;
};

CO_NAMESPACE_END
