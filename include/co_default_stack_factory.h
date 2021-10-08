#pragma once
#include "co_singleton.h"
#include "co_stack_factory.h"

class co_default_stack_factory final : public co_stack_factory,
                                       public co_singleton<co_default_stack_factory>
{
public:
    co_stack* create_stack(size_t size) override;
    void      destroy_stack(co_stack* stack) override;
    void      set_manager(co_manager* manager) override;
};
