_Pragma("once");
#include "cocpp/core/co_ctx.h"
#include "cocpp/utils/co_any.h"
#include "cocpp/utils/co_singleton.h"

#include <functional>

CO_NAMESPACE_BEGIN

struct co_ctx_config;

class co_ctx_factory final
{

public:
    static co_ctx *create_ctx(const co_ctx_config &config, std::function<void(co_any &)> entry);
    static void destroy_ctx(co_ctx *ctx);
};

CO_NAMESPACE_END