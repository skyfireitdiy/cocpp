#pragma once
#include "co_ctx_config.h"
#include "co_object_pool.h"
#include "co_singleton.h"
#include "co_stack.h"

CO_NAMESPACE_BEGIN
class co_ctx_factory final : public co_singleton<co_ctx_factory>
{

private:
    co_object_pool<co_ctx> ctx_pool__;

public:
    co_ctx* create_ctx(const co_ctx_config& config);
    void    destroy_ctx(co_ctx* ctx);
    void    free_obj_pool();
};

CO_NAMESPACE_END