#pragma once
#include "co_ctx_config.h"
#include "co_ctx_factory.h"

class co_default_ctx_factory : public co_ctx_factory,
                               public co_singleton<co_default_ctx_factory>
{
private:
    co_manager* manager__ = nullptr;

public:
    co_ctx* create_ctx(const co_ctx_config& config) override;
    void    destroy_ctx(co_ctx* ctx) override;
    void    set_manager(co_manager* manager) override;
};