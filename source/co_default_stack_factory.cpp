#include "co_default_stack_factory.h"
#include "co_default_stack.h"

co_stack* co_default_stack_factory::create_stack(size_t size)
{
    return new co_default_stack(size);
}

void co_default_stack_factory::destroy_stack(co_stack* stack)
{
    delete stack;
}
