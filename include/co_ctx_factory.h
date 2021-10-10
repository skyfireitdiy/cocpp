#pragma once
#include "co_ctx_config.h"
#include "co_ctx_factory.h"
#include "co_singleton.h"

class co_ctx_factory final : public co_singleton<co_ctx_factory>
{
private:
    co_manager* manager__ = nullptr;

public:
    co_ctx* create_ctx(const co_ctx_config& config);
    void    destroy_ctx(co_ctx* ctx);
    void    set_manager(co_manager* manager);
};