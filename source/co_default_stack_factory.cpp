#include "co_default_stack_factory.h"
#include "co_default_stack.h"
#include "co_define.h"

co_stack* co_default_stack_factory::create_stack(size_t size)
{
    auto ret = new co_default_stack(size);
    CO_DEBUG("create stack %p", ret);
    return ret;
}

void co_default_stack_factory::destroy_stack(co_stack* stack)
{
    CO_DEBUG("destory stack %p", stack);
    delete stack;
}

void co_default_stack_factory::set_manager(co_manager* manager)
{
}