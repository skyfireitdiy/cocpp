#pragma once
#include "co_ctx_config.h"
#include "co_singleton.h"

CO_NAMESPACE_BEGIN
class co_ctx_factory final : public co_singleton<co_ctx_factory>
{
public:
    co_ctx* create_ctx(const co_ctx_config& config);
    void    destroy_ctx(co_ctx* ctx);
};

CO_NAMESPACE_END