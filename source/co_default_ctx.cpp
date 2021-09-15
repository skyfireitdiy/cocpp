#include "co_default_ctx.h"

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
    state__ = state;
}

co_byte** co_default_ctx::regs()
{
    return reinterpret_cast<co_byte**>(&regs__);
}

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
{
    init_regs__();
}

void co_default_ctx::set_detach()
{
    detached__ = true;
}

bool co_default_ctx::detach()
{
    return detached__;
}