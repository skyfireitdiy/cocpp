#include "co.h"
#include "co_ctx.h"
#include "co_defer.h"
#include "co_define.h"
#include "co_env.h"
#include "co_type.h"
#include "co_vos.h"

#include <signal.h>
#include <sys/syscall.h>
#include <unistd.h>

#ifdef __GNUC__
#ifdef __x86_64__

CO_NAMESPACE_BEGIN

static void          switch_signal_handler(int signo);
static constexpr int CO_SWITCH_SIGNAL = 10;

struct sigcontext_64
{
    unsigned long long r8;    // 0*8
    unsigned long long r9;    // 1*8
    unsigned long long r10;   // 2*8
    unsigned long long r11;   // 3*8
    unsigned long long r12;   // 4*8
    unsigned long long r13;   // 5*8
    unsigned long long r14;   // 6*8
    unsigned long long r15;   // 7*8
    unsigned long long di;    // 8*8
    unsigned long long si;    // 9*8
    unsigned long long bp;    // 10*8
    unsigned long long bx;    // 11*8
    unsigned long long dx;    // 12*8
    unsigned long long ax;    // 13*8
    unsigned long long cx;    // 14*8
    unsigned long long sp;    // 15*8
    unsigned long long ip;    // 16*8
    unsigned long long flags; // 17*8

    // 后面这部分获取不到，暂时不用

    // unsigned short     cs;           //18*8
    // unsigned short     gs;           //18*8+2
    // unsigned short     fs;           //18*8+4
    // unsigned short     __pad0;       //18*8+6
    // unsigned long long err;          //19*8
    // unsigned long long trapno;       //20*8
    // unsigned long long oldmask;      //21*8
    // unsigned long long cr2;          //22*8
    // unsigned long long fpstate;      //23*8
    // unsigned long long reserved1[8]; //24*8
};

static sigcontext_64* get_sigcontext_64(co_ctx* ctx)
{
    return reinterpret_cast<sigcontext_64*>(ctx->regs());
}

#define CO_SETREG(context, x, y) context->x = reinterpret_cast<decltype(context->x)>(y)

co_byte* get_rsp(co_ctx* ctx)
{
    return reinterpret_cast<co_byte*>(get_sigcontext_64(ctx)->sp);
}

void switch_to(co_byte** curr, co_byte** next)
{

    __asm volatile("" ::
                       : "memory");

    __asm volatile("popq %rbp");

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

    __asm volatile("pushq %rbp");

    __asm volatile("" ::
                       : "memory");
}

void init_ctx(co_stack* shared_stack, co_ctx* ctx)
{
    auto      context = get_sigcontext_64(ctx);
    co_stack* stack   = ctx->stack();
    if (ctx->test_flag(CO_CTX_FLAG_SHARED_STACK))
    {
        stack = shared_stack;
    }
    auto config = ctx->config();

    CO_SETREG(context, sp, stack->stack_top());
    CO_SETREG(context, bp, stack->stack_top());
    CO_SETREG(context, ip, &co_ctx::real_entry);
    CO_SETREG(context, di, ctx);
}

co_tid gettid()
{
    return static_cast<co_tid>(::gettid());
}

void setup_switch_handler()
{
    ::signal(CO_SWITCH_SIGNAL, switch_signal_handler);
}

void send_switch_from_outside_signal(co_env* env)
{
    ::syscall(SYS_tgkill, ::getpid(), static_cast<pid_t>(env->schedule_thread_tid()), CO_SWITCH_SIGNAL);
}

static void save_context_to_ctx(sigcontext_64* context, co_ctx* ctx)
{
    *reinterpret_cast<sigcontext_64*>(ctx->regs()) = *context;
}

static void restore_context_from_ctx(sigcontext_64* context, co_ctx* ctx)
{
    *context = *reinterpret_cast<sigcontext_64*>(ctx->regs());
}

void switch_from_outside(sigcontext_64* context)
{
    auto env = co::current_env();

    if (!env->safepoint())
    {
        return;
    }

    co_ctx* curr = nullptr;
    co_ctx* next = nullptr;

    if (!env->prepare_to_switch(curr, next))
    {
        return;
    }
    save_context_to_ctx(context, curr);
    restore_context_from_ctx(context, next);
    // 能调用到此处，说明当前一定是在安全点内
}

static void switch_signal_handler(int signo)
{
    sigcontext_64* context = nullptr;
    __asm volatile("leaq 88(%%rsp), %%rax\t\n"
                   "movq %%rax, %0\t\n"
                   : "=m"(context)
                   :
                   : "memory", "rax");
    switch_from_outside(context);
}

CO_NAMESPACE_END

#endif
#endif