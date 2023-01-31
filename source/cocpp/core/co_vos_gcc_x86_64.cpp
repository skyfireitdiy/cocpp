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

static sigcontext_64 *get_sigcontext_64(co_ctx *ctx)
{
    return reinterpret_cast<sigcontext_64 *>(ctx->regs());
}

#define CO_SETREG(context, x, y) context->x = reinterpret_cast<decltype(context->x)>(y)

co_byte *get_rsp(co_ctx *ctx)
{
    return reinterpret_cast<co_byte *>(get_sigcontext_64(ctx)->sp);
}

__attribute__((naked)) void switch_to(co_byte **curr, co_byte **next)
{

    __asm volatile("" ::
                       : "memory");


    __asm volatile("movq %r8, 0(%rdi)");
    __asm volatile("movq %r9, 8(%rdi)");
    __asm volatile("movq %r10, 16(%rdi)");
    __asm volatile("movq %r11, 24(%rdi)");
    __asm volatile("movq %r12, 32(%rdi)");
    __asm volatile("movq %r13, 40(%rdi)");
    __asm volatile("movq %r14, 48(%rdi)");
    __asm volatile("movq %r15, 56(%rdi)");
    __asm volatile("movq %rdi, 64(%rdi)");
    __asm volatile("movq %rsi, 72(%rdi)");
    __asm volatile("movq %rbp, 80(%rdi)");
    __asm volatile("movq %rbx, 88(%rdi)");
    __asm volatile("movq %rdx, 96(%rdi)");
    __asm volatile("movq %rax, 104(%rdi)");
    __asm volatile("movq %rcx, 112(%rdi)");
    __asm volatile("popq 128(%rdi)");
    __asm volatile("pushf");
    __asm volatile("popq 136(%rdi)");

    __asm volatile("movq %rsp, 120(%rdi)"); // rsp必须在rip后保存，先恢复
    /////////////////////////////////////////////////
    __asm volatile("movq 120(%rsi), %rsp");

    __asm volatile("pushq 136(%rsi)");
    __asm volatile("popf");
    __asm volatile("pushq 128(%rsi)");
    __asm volatile("movq 112(%rsi), %rcx");
    __asm volatile("movq 104(%rsi), %rax");
    __asm volatile("movq 96(%rsi), %rdx");
    __asm volatile("movq 88(%rsi), %rbx");
    __asm volatile("movq 80(%rsi), %rbp");
    __asm volatile("movq 64(%rsi), %rdi");
    __asm volatile("movq 56(%rsi), %r15");
    __asm volatile("movq 48(%rsi), %r14");
    __asm volatile("movq 40(%rsi), %r13");
    __asm volatile("movq 32(%rsi), %r12");
    __asm volatile("movq 24(%rsi), %r11");
    __asm volatile("movq 16(%rsi), %r10");
    __asm volatile("movq 8(%rsi), %r9");
    __asm volatile("movq 0(%rsi), %r8");
    __asm volatile("movq 72(%rsi), %rsi"); // 必须放在最后恢复

    __asm volatile("" ::
                       : "memory");

    __asm volatile("retq");
}

void init_ctx(co_stack *stack, co_ctx *ctx)
{
    auto context = get_sigcontext_64(ctx);
    CO_SETREG(context, sp, stack->stack_top() - 8);
    CO_SETREG(context, bp, stack->stack_top() - 8);
    CO_SETREG(context, ip, &co_ctx::real_entry);
    CO_SETREG(context, di, ctx);
    // Store the address of '_start' at the bottom of the stack to prevent segment fault in case of exception backtracking of the call chain
    *reinterpret_cast<void **>(context->sp) = reinterpret_cast<void *>(&::_start);
}

co_tid gettid()
{
    return static_cast<co_tid>(::gettid());
}

void send_switch_from_outside_signal(co_env *env)
{
    ::syscall(SYS_tgkill, ::getpid(), static_cast<pid_t>(env->schedule_thread_tid()), CO_SWITCH_SIGNAL);
}

static void save_context_to_ctx(sigcontext_64 *context, co_ctx *ctx)
{
    *reinterpret_cast<sigcontext_64 *>(ctx->regs()) = *context;
    // printf("rip: 0x%llx\n", context->ip);
    // printf("rsp: 0x%llx\n", context->sp);
    // printf("rbp: 0x%llx\n", context->bp);
    // print_backtrace();
}

static void restore_context_from_ctx(sigcontext_64 *context, co_ctx *ctx)
{
    *context = *reinterpret_cast<sigcontext_64 *>(ctx->regs());
}

void switch_from_outside(sigcontext_64 *context)
{
    auto env = CoCurrentEnv();

    co_ctx *curr = nullptr;
    co_ctx *next = nullptr;
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
