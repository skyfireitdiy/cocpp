#include "co_default_env.h"
#include "co_ctx_factory.h"
#include "co_define.h"
#include "co_env_factory.h"
#include "co_manager.h"
#include "co_scheduler.h"
#include "co_type.h"

#include <cassert>
#include <future>
#include <iterator>

#ifdef _MSC_VER
#ifdef _WIN64
extern "C" void __switch_to_msvc_x64(co_byte**, co_byte**);
#else
#error only support x86_64 in msvc
#endif
#endif

co_default_env::co_default_env(co_scheduler* scheduler, co_ctx* idle_ctx, co_stack* shared_stack, bool create_new_thread)
    : scheduler__(scheduler)
    , shared_stack__(shared_stack)
    , idle_ctx__(idle_ctx)
    , state__(co_env_state::created)
{
    if (create_new_thread)
    {
        // 此处设置状态是防止 add_ctx 前调度线程还未准备好，状态断言失败
        set_state(co_env_state::idle);
        start_schedule();
    }
    else
    {
        current_env__ = this;
    }
}

co_stack* co_default_env::shared_stack() const
{
    return shared_stack__;
}

void co_default_env::add_ctx(co_ctx* ctx)
{
    assert(ctx != nullptr);
    assert(state__ != co_env_state::created && state__ != co_env_state::destorying);
    {
        std::lock_guard<std::mutex> lock(mu_wake_up_idle__);
        ctx->set_env(this);
        scheduler__->add_ctx(ctx); // 添加到调度器
        set_state(co_env_state::busy);
    }
    CO_DEBUG("add ctx weak up idle co");
    cond_wake_schedule__.notify_one();
}

std::optional<co_ret> co_default_env::wait_ctx(co_ctx* ctx, const std::chrono::nanoseconds& timeout)
{
    std::optional<co_ret> ret;
    auto                  now = std::chrono::high_resolution_clock::now();
    while (ctx->state() != co_state::finished)
    {
        if (std::chrono::high_resolution_clock::now() - now > timeout)
        {
            return ret;
        }
        schedule_switch();
    }
    return ctx->ret_ref();
}

co_ret co_default_env::wait_ctx(co_ctx* ctx)
{
    while (ctx->state() != co_state::finished)
    {
        schedule_switch();
        CO_DEBUG("ctx %s %p state: %d", ctx->config().name.c_str(), ctx, ctx->state());
    }
    return ctx->ret_ref();
}

int co_default_env::workload() const
{
    return scheduler__->count();
}

bool co_default_env::has_scheduler_thread() const
{
    std::shared_lock<std::shared_mutex> lock(mu_state__);
    return state__ != co_env_state::created;
}

co_env_state co_default_env::state() const
{
    std::shared_lock<std::shared_mutex> lock(mu_state__);
    return state__;
}

void co_default_env::set_state(co_env_state state)
{
    std::unique_lock<std::shared_mutex> lock(mu_state__);
    if (state__ != co_env_state::destorying) // destorying 状态不允许迁移到其他状态
    {
        state__ = state;
    }
}

void co_default_env::remove_detached_ctx__()
{
    auto curr    = scheduler__->current_ctx();
    auto all_ctx = scheduler__->all_ctx();
    for (auto& ctx : all_ctx)
    {
        // 注意：此处不能删除当前的ctx，如果删除了，switch_to的当前上下文就没地方保存了
        if (ctx->state() == co_state::finished && !ctx->test_flag(CO_CTX_FLAG_HANDLE_BY_CO) && ctx != curr)
        {
            scheduler__->remove_ctx(ctx);
            manager__->ctx_factory()->destroy_ctx(ctx);
        }
    }
}

void co_default_env::schedule_switch()
{
    remove_detached_ctx__();

    auto curr = current_ctx();
    assert(curr != nullptr);

    co_ctx* next = nullptr;
    // 如果要销毁、并且可以销毁，切换到idle销毁
    if (state() == co_env_state::destorying)
    {
        next = idle_ctx__;
    }
    else
    {
        next = scheduler__->choose_ctx();
        if (next == nullptr)
        {
            next = idle_ctx__;
        }
    }

    if (curr->state() != co_state::finished)
    {
        curr->set_state(co_state::suspended);
    }

    // CO_DEBUG("from %p to %p", curr, next);

    next->set_state(co_state::running);
    if (curr == next)
    {
        return;
    }

    if (next != idle_ctx__)
    {
        set_state(co_env_state::busy);
    }
    switch_to__(curr->regs(), next->regs());
}

void co_default_env::remove_ctx(co_ctx* ctx)
{
    scheduler__->remove_ctx(ctx);
    manager__->ctx_factory()->destroy_ctx(ctx);
    update_state__();
}

void co_default_env::switch_to__(co_byte** curr, co_byte** next)
{
#ifdef _MSC_VER
#ifdef _WIN64
    ::__switch_to_msvc_x64(curr, next);
#else
#error only support x86_64 in msvc
#endif
#endif

#ifdef __GNUC__
#ifdef __x86_64__
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
#else
#error only supported x86_64
#endif
#endif
}

co_ctx* co_default_env::current_ctx() const
{
    auto ret = scheduler__->current_ctx();
    if (ret == nullptr)
    {
        return idle_ctx__;
    }
    return ret;
}

co_ctx* co_default_env::idle_ctx() const
{
    return idle_ctx__;
}

void co_default_env::stop_schedule()
{
    CO_DEBUG("set env to destorying");
    // created 状态说明没有调度线程，不能转为destroying状态
    {
        std::lock_guard<std::mutex> lock(mu_wake_up_idle__);
        if (state() != co_env_state::created)
        {
            set_state(co_env_state::destorying);
        }
    }
    CO_DEBUG("stop schedule wake up idle co");
    cond_wake_schedule__.notify_one(); // 唤醒idle的ctx
}

void co_default_env::start_schedule()
{
    worker__ = std::async([this]() {
        current_env__ = this;
        start_schedule_routine__();
    });
}

void co_default_env::start_schedule_routine__()
{
    set_state(co_env_state::idle);
    while (state() != co_env_state::destorying)
    {
        schedule_switch();
        // 切回idle之后，直接睡眠等待
        {
            std::unique_lock<std::mutex> lock(mu_wake_up_idle__);
            if (state() == co_env_state::destorying)
            {
                break;
            }
            CO_DEBUG("idle co start wait add ctx or destroying ...");
            cond_wake_schedule__.wait(lock);
            if (state() == co_env_state::destorying)
            {
                break;
            }
        }
    }

    CO_DEBUG("stop schedule, prepare to cleanup");

    remove_all_ctx__();
    remove_current_env__();
}

void co_default_env::schedule_in_this_thread()
{
    start_schedule_routine__();
}

void co_default_env::set_manager(co_manager* manager)
{
    manager__ = manager;
}

co_manager* co_default_env::manager() const
{
    return manager__;
}

void co_default_env::remove_all_ctx__()
{
    auto all_ctx = scheduler__->all_ctx();
    for (auto& ctx : all_ctx)
    {
        remove_ctx(ctx);
    }
}

void co_default_env::remove_current_env__()
{
    if (current_env__ != nullptr)
    {
        assert(manager__ != nullptr);
        CO_DEBUG("add self to clean up list: %p", current_env__);
        manager__->remove_env(current_env__);
        current_env__ = nullptr;
    }
}

void co_default_env::update_state__()
{
    // 只有一个ctx的时候，是idle_ctx，所以设置为空闲状态
    if (scheduler__->current_ctx() == nullptr)
    {
        set_state(co_env_state::idle);
    }
}

co_scheduler* co_default_env::scheduler() const
{
    return scheduler__;
}