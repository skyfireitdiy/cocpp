_Pragma("once");
#include "cocpp/core/co_ctx.h"
#include "cocpp/utils/co_any.h"
#include "cocpp/utils/co_singleton.h"

#include <functional>

CO_NAMESPACE_BEGIN

struct co_ctx_config;

class co_ctx_factory final : public co_singleton<co_ctx_factory>
{

private:
    co_object_pool<co_ctx> ctx_pool__ { MAX_CTX_CACHE_COUNT };

public:
    co_ctx* create_ctx(const co_ctx_config& config, std::function<void(co_any&)> entry);
    void    destroy_ctx(co_ctx* ctx);
    void    free_obj_pool();
};

CO_NAMESPACE_END