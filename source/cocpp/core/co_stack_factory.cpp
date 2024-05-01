#include "cocpp/core/co_stack_factory.h"
#include "cocpp/core/co_define.h"
#include "cocpp/core/co_env.h"
#include "cocpp/core/co_manager.h"
#include "cocpp/core/co_stack.h"
#include "cocpp/core/co_type.h"
#include "cocpp/core/co_vos.h"

CO_NAMESPACE_BEGIN

co_stack *co_stack_factory::create_stack(size_t size)
{
    co_byte *mem = nullptr;
    if (size != 0)
    {
        mem = reinterpret_cast<co_byte *>(alloc_mem_by_mmap(size));
        if (nullptr == mem || reinterpret_cast<co_byte *>(-1) == mem)
        {
            throw std::runtime_error("mmap mem failed");
        }
        if (!set_mem_dontneed(mem, size))
        {
            CO_WARN("set mem dontneed failed");
        }
    }
    auto ret = new co_stack(mem, size);
    if (ret == nullptr)
    {
        free_mem_by_munmap(mem, size);
        throw std::bad_alloc();
    }
    return ret;
}

void co_stack_factory::destroy_stack(co_stack *stack)
{
    CoPreemptGuard();
    if (stack->stack_size() != 0)
    {
        free_mem_by_munmap(stack->stack(), stack->stack_size());
    }
    delete stack;
}

CO_NAMESPACE_END
