#include "co_default_ctx.h"
#include "co_define.h"
#include <cassert>
#include <cstring>

#ifdef _MSC_VER
#ifdef _WIN64
extern "C" void __get_mxcsr_msvc_x64(co_byte**);
extern "C" void __get_fcw_msvc_x64(co_byte**);
extern "C" void __get_TEB_8_msvc_x64(co_byte**);
extern "C" void __get_TEB_16_msvc_x64(co_byte**);
extern "C" void __get_TEB_24_msvc_x64(co_byte**);
extern "C" void __get_TEB_5240_msvc_x64(co_byte**);
#endif
#endif

co_stack* co_default_ctx::stack() const
{
    return stack__;
}

co_state co_default_ctx::state() const
{
    return state__;
}

void co_default_ctx::set_state(co_state state)
{
    // if (config__.name != "idle")
    // {
    //     CO_DEBUG("set state %u to %s %p\n", state, config__.name.c_str(), this);
    // }
    state__ = state;
}

co_byte** co_default_ctx::regs()
{
    return reinterpret_cast<co_byte**>(&regs__);
}

#ifdef __GNUC__
#ifdef __x86_64__
static void __get_mxcsr_gcc_x64(void*)
{
    __asm volatile(
        "stmxcsr (%%rdi)\n"
        :
        :
        : "memory");
}

static void __get_fcw_gcc_x64(void*)
{
    __asm volatile(
        "fnstcw (%%rdi)\n"
        :
        :
        : "memory");
}
#endif
#endif

void co_default_ctx::init_regs__()
{
#ifdef _MSC_VER
#ifdef _WIN64
    memset(stack__->stack(), 0, stack__->stack_size());
    regs__[reg_index_RSP__] = stack__->stack_top() - sizeof(void*); // why sub sizeof(void*) ?
    regs__[reg_index_RBP__] = regs__[reg_index_RSP__];
    regs__[reg_index_RIP__] = reinterpret_cast<co_byte*>(config__.startup);
    regs__[reg_index_RCX__] = reinterpret_cast<co_byte*>(this);

    __get_mxcsr_msvc_x64(&regs__[reg_index_MXCSR__]);
    __get_fcw_msvc_x64(&regs__[reg_index_FCW__]);
    __get_TEB_8_msvc_x64(&regs__[reg_index_TEB_8__]);
    __get_TEB_16_msvc_x64(&regs__[reg_index_TEB_16__]);
    __get_TEB_24_msvc_x64(&regs__[reg_index_TEB_24__]);
    __get_TEB_5240_msvc_x64(&regs__[reg_index_TEB_5240__]);

#else
#error only support x86_64 in msvc
#endif
#endif

#ifdef __GNUC__
#ifdef __x86_64__
    memset(stack__->stack(), 0, stack__->stack_size());
    regs__[reg_index_RSP__] = stack__->stack_top() - sizeof(void*); // why sub sizeof(void*) ?
    regs__[reg_index_RBP__] = regs__[reg_index_RSP__];
    regs__[reg_index_RIP__] = reinterpret_cast<co_byte*>(config__.startup);
    regs__[reg_index_RDI__] = reinterpret_cast<co_byte*>(this);

    __get_mxcsr_gcc_x64(&regs__[reg_index_MXCSR__]);
    __get_fcw_gcc_x64(&regs__[reg_index_MXCSR__]);
#else
#error only supported x86_64
#endif
#endif
}

const co_ctx_config& co_default_ctx::config() const
{
    return config__;
}

std::any& co_default_ctx::ret_ref()
{
    return ret__;
}

void co_default_ctx::set_env(co_env* env)
{
    env__ = env;
}

co_env* co_default_ctx::env() const
{
    return env__;
}

co_default_ctx::co_default_ctx(co_stack* stack, const co_ctx_config& config)
    : stack__(stack)
    , state__(co_state::suspended)
    , config__(config)
    , priority__(config.priority)
{
    if (priority__ >= CO_MAX_PRIORITY)
    {
        priority__ = CO_MAX_PRIORITY - 1;
    }
    if (priority__ < 0)
    {
        priority__ = 0;
    }

    init_regs__();
}

void co_default_ctx::set_flag(int flag)
{
    assert(flag < CO_CTX_FLAG_MAX);
    std::lock_guard<std::mutex> lck(mu_flag__);
    flag__.set(flag);
}

bool co_default_ctx::test_flag(int flag)
{
    assert(flag < CO_CTX_FLAG_MAX);
    std::lock_guard<std::mutex> lck(mu_flag__);
    return flag__.test(flag);
}

void co_default_ctx::reset_flag(int flag)
{
    assert(flag < CO_CTX_FLAG_MAX);
    std::lock_guard<std::mutex> lck(mu_flag__);
    flag__.reset(flag);
}

int co_default_ctx::priority() const
{
    return priority__;
}

void co_default_ctx::set_priority(int priority)
{
    priority__ = priority;
}