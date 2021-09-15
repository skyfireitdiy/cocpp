#pragma once
#include "co_ctx_config.h"
#include "co_ctx_factory.h"
#include "co_default_stack_factory.h"

class co_default_ctx_factory : public co_ctx_factory,
                               public co_singleton<co_default_ctx_factory>
{
private:
    co_stack_factory* stack_factory__ = co_default_stack_factory::instance();

public:
    co_ctx* create_ctx(const co_ctx_config& config) override;
    void    destroy_ctx(co_ctx* ctx) override;
    void    set_stack_factory(co_stack_factory* stack_factory) override;
};