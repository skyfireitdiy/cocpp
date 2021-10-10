#pragma once
#include "co_nocopy.h"
#include "co_singleton.h"

#include <cstddef>

class co_stack;
class co_manager;

class co_stack_factory final : public co_singleton<co_stack_factory>
{
public:
    co_stack* create_stack(size_t size);
    void      destroy_stack(co_stack* stack);
    void      set_manager(co_manager* manager);
};
