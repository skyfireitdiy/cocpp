#include "cocpp/core/co_ctx.h"
#include "cocpp/core/co_define.h"
#include "cocpp/core/co_env.h"
#include "cocpp/core/co_manager.h"
#include "cocpp/core/co_stack.h"
#include "cocpp/core/co_type.h"
#include "cocpp/core/co_vos.h"
#include "cocpp/exception/co_exception.h"
#include "cocpp/utils/co_defer.h"

#include <signal.h>
#include <sys/syscall.h>
#include <unistd.h>

#ifdef __GNUC__
#ifdef __x86_64__

extern "C" void _start();
CO_NAMESPACE_BEGIN

void init_ctx(co_stack* stack, co_ctx* ctx)
{
    auto& context = ctx->context();
    getcontext(&context);
    context.uc_stack.ss_sp    = stack->stack();
    context.uc_stack.ss_size  = stack->stack_size();
    context.uc_stack.ss_flags = 0;
    context.uc_link           = nullptr;
    makecontext(&context, reinterpret_cast<void (*)()>(&co_ctx::real_entry), 1, ctx);
}

co_tid gettid()
{
    return static_cast<co_tid>(::gettid());
}

void send_switch_from_outside_signal(co_env* env)
{
    ::syscall(SYS_tgkill, ::getpid(), static_cast<pid_t>(env->schedule_thread_tid()), CO_SWITCH_SIGNAL);
}

static void save_context_to_ctx(ucontext_t& context, co_ctx* ctx)
{
    // *reinterpret_cast<sigcontext_64*>(ctx->context()) = *context;
    ctx->context() = context;
    // printf("rip: 0x%llx\n", context->ip);
    // printf("rsp: 0x%llx\n", context->sp);
    // printf("rbp: 0x%llx\n", context->bp);
    // print_backtrace();
}

static void restore_context_from_ctx(ucontext_t& context, co_ctx* ctx)
{
    context = ctx->context();
}

void switch_from_outside(ucontext_t& context)
{
    auto env = CoCurrentEnv();

    co_ctx* curr = nullptr;
    co_ctx* next = nullptr;
    {
        if (!env->can_force_schedule())
        {
            return;
        }
        if (!env->try_lock_schedule())
        {
            return;
        }
        CoDefer(env->unlock_schedule());
        if (!env->prepare_to_switch(curr, next))
        {
            return;
        }
    }
    save_context_to_ctx(context, curr);
    restore_context_from_ctx(context, next);
}

CO_NAMESPACE_END

#endif
#endif