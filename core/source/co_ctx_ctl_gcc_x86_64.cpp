#include "co_ctx_ctl_gcc_x86_64.h"
#include "co_define.h"

#ifdef __GNUC__
#ifdef __x86_64__

CO_NAMESPACE_BEGIN

void switch_to__(co_byte** curr, co_byte** next)
{

    __asm volatile("" ::
                       : "memory");

    __asm volatile("popq %rbp");

    __asm volatile("movq %rdi, (%rdi)");
    __asm volatile("popq 8(%rdi)");
    __asm volatile("movq %rsp, 16(%rdi)");
    __asm volatile("movq %rbp, 24(%rdi)");
    __asm volatile("movq %rbx, 32(%rdi)");
    __asm volatile("movq %r12, 40(%rdi)");
    __asm volatile("movq %r13, 48(%rdi)");
    __asm volatile("movq %r14, 56(%rdi)");
    __asm volatile("movq %r15, 64(%rdi)");
    __asm volatile("stmxcsr 72(%rdi)");
    __asm volatile("fnstcw 80(%rdi)");

    __asm volatile("fldcw 80(%rsi)");
    __asm volatile("ldmxcsr 72(%rsi)");
    __asm volatile("movq 64(%rsi), %r15");
    __asm volatile("movq 56(%rsi), %r14");

    __asm volatile("movq 48(%rsi), %r13");
    __asm volatile("movq 40(%rsi), %r12");
    __asm volatile("movq 32(%rsi), %rbx");
    __asm volatile("movq 24(%rsi), %rbp");
    __asm volatile("movq 16(%rsi), %rsp");
    __asm volatile("pushq 8(%rsi)");
    __asm volatile("movq (%rsi), %rdi");

    __asm volatile("pushq %rbp");

    __asm volatile("" ::
                       : "memory");
}

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

void init_ctx__(co_stack* shared_stack, co_ctx* ctx)
{
    auto      regs  = ctx->regs();
    co_stack* stack = ctx->stack();
    if (ctx->test_flag(CO_CTX_FLAG_SHARED_STACK))
    {
        stack = shared_stack;
    }
    auto config = ctx->config();

    regs[reg_index_RSP__] = stack->stack_top();
    regs[reg_index_RBP__] = regs[reg_index_RSP__];
    regs[reg_index_RIP__] = reinterpret_cast<co_byte*>(co_entry);
    regs[reg_index_RDI__] = reinterpret_cast<co_byte*>(ctx);

    __get_mxcsr_gcc_x64(&regs[reg_index_MXCSR__]);
    __get_fcw_gcc_x64(&regs[reg_index_FCW__]);
}

CO_NAMESPACE_END

#endif
#endif