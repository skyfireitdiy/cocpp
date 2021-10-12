#include "co_env.h"
#include "co_ctx.h"
#include "co_ctx_factory.h"
#include "co_defer.h"
#include "co_define.h"
#include "co_env_factory.h"
#include "co_manager.h"
#include "co_scheduler.h"
#include "co_stack.h"
#include "co_type.h"

#include <cassert>
#include <cstring>
#include <future>
#include <iterator>
#include <mutex>

CO_NAMESPACE_BEGIN

thread_local co_env* current_env__ = nullptr;

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

#ifdef _MSC_VER
#ifdef _WIN64
extern "C" void __switch_to_msvc_x64(co_byte**, co_byte**);
#else
#error only support x86_64 in msvc
#endif
#endif

co_env::co_env(co_scheduler* scheduler, co_stack* shared_stack, co_ctx* idle_ctx, bool create_new_thread)
    : scheduler__(scheduler)
    , shared_stack__(shared_stack)
    , idle_ctx__(idle_ctx)
    , state__(co_env_state::created)
{
    idle_ctx__->set_env(this);
    if (create_new_thread)
    {
        // 此处设置状态是防止 add_ctx 前调度线程还未准备好，状态断言失败
        set_state(co_env_state::idle);
        start_schedule();
    }
    else
    {
        set_flag(CO_ENV_FLAG_NO_SCHE_THREAD);
        set_flag(CO_ENV_FLAG_COVERTED);
        current_env__ = this;
    }
}

void co_env::add_ctx(co_ctx* ctx)
{
    assert(ctx != nullptr);
    assert(state__ != co_env_state::created && state__ != co_env_state::destorying);

    std::lock_guard<std::recursive_mutex> lock(mu_wake_up_idle__);
    ctx->set_env(this);
    scheduler__->add_ctx(ctx); // 添加到调度器
    set_state(co_env_state::busy);

    // CO_O_DEBUG("add ctx wake up env: %p", this);
    wake_up();
}

std::optional<co_return_value> co_env::wait_ctx(co_ctx* ctx, const std::chrono::nanoseconds& timeout)
{
    // 反转优先级，防止高优先级ctx永久等待低优先级ctx
    auto old_priority = current_ctx()->priority();
    current_ctx()->set_priority(ctx->priority());
    CoDefer([this, old_priority] {
        current_ctx()->set_priority(old_priority);
    });

    std::optional<co_return_value> ret;

    auto now = std::chrono::high_resolution_clock::now();
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

co_return_value co_env::wait_ctx(co_ctx* ctx)
{
    auto old_priority = current_ctx()->priority();
    current_ctx()->set_priority(ctx->priority());

    CoDefer([this, old_priority] {
        current_ctx()->set_priority(old_priority);
    });

    while (ctx->state() != co_state::finished)
    {
        schedule_switch();
        // // CO_O_DEBUG("ctx %s %p state: %d", ctx->config().name.c_str(), ctx, ctx->state());
    }
    return ctx->ret_ref();
}

int co_env::workload() const
{
    return scheduler__->count();
}

bool co_env::has_scheduler_thread() const
{
    return !test_flag(CO_ENV_FLAG_NO_SCHE_THREAD);
}

co_env_state co_env::state() const
{
    std::shared_lock<std::shared_mutex> lock(mu_state__);
    return state__;
}

void co_env::set_state(co_env_state state)
{
    std::unique_lock<std::shared_mutex> lock(mu_state__);
    if (state__ != co_env_state::destorying) // destorying 状态不允许迁移到其他状态
    {
        state__ = state;
    }
}

void co_env::remove_detached_ctx__()
{
    auto curr    = scheduler__->current_ctx();
    auto all_ctx = scheduler__->all_ctx();
    for (auto& ctx : all_ctx)
    {
        // 注意：此处不能删除当前的ctx，如果删除了，switch_to的当前上下文就没地方保存了
        if (ctx->state() == co_state::finished && ctx->can_destroy() && ctx != curr)
        {
            scheduler__->remove_ctx(ctx);
            ctx_factory__->destroy_ctx(ctx);
        }
    }
}

co_ctx* co_env::next_ctx__()
{
    // 如果要销毁、并且可以销毁，切换到idle销毁
    if (state() == co_env_state::destorying)
    {
        return idle_ctx__;
    }
    else
    {
        auto next = scheduler__->choose_ctx();
        return next == nullptr ? idle_ctx__ : next;
    }
}

void co_env::update_ctx_state__(co_ctx* curr, co_ctx* next)
{
    // 如果当前运行的ctx已经完成，状态不变
    if (curr->state() != co_state::finished)
    {
        curr->set_state(co_state::suspended);
    }
    // // CO_O_DEBUG("from %p to %p", curr, next);
    next->set_state(co_state::running);
}

void co_env::schedule_switch()
{
    co_ctx* curr = nullptr;
    co_ctx* next = nullptr;
    {
        // 切换前加锁
        std::lock_guard<std::mutex> lock(mu_schedule__);

        set_flag(CO_ENV_FLAG_SCHEDULED);

        remove_detached_ctx__();

        curr = current_ctx();
        next = next_ctx__();

        assert(curr != nullptr);
        assert(next != nullptr);

        if (next->state() == co_state::created) // 第一次运行需要初始化
        {
            init_ctx(next);
        }

        update_ctx_state__(curr, next);
        if (curr == next)
        {
            return;
        }
        if (next != idle_ctx__)
        {
            set_state(co_env_state::busy);
        }

        if (curr->test_flag(CO_CTX_FLAG_SHARED_STACK) || next->test_flag(CO_CTX_FLAG_SHARED_STACK))
        {
            shared_stack_switch_context__.from        = curr;
            shared_stack_switch_context__.to          = next;
            shared_stack_switch_context__.need_switch = true;
            next                                      = idle_ctx__;
        }
    }
    switch_to__(curr->regs(), next->regs());
}

void co_env::remove_ctx(co_ctx* ctx)
{
    scheduler__->remove_ctx(ctx);
    ctx_factory__->destroy_ctx(ctx);
}

void co_env::switch_to__(co_byte** curr, co_byte** next)
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

co_ctx* co_env::current_ctx() const
{
    auto ret = scheduler__->current_ctx();
    if (ret == nullptr)
    {
        return idle_ctx__;
    }
    return ret;
}

void co_env::stop_schedule()
{
    // CO_O_DEBUG("set env to destorying, %p", this);
    // created 状态说明没有调度线程，不能转为destroying状态

    std::lock_guard<std::recursive_mutex> lock(mu_wake_up_idle__);
    if (!test_flag(CO_ENV_FLAG_NO_SCHE_THREAD))
    {
        set_state(co_env_state::destorying);
    }

    // CO_O_DEBUG("%p : stop schedule wake up idle co", this);
    wake_up(); // 唤醒idle的ctx
}

void co_env::start_schedule()
{
    worker__ = std::async([this]() {
        current_env__ = this;
        start_schedule_routine__();
    });
}

void co_env::start_schedule_routine__()
{
    reset_flag(CO_ENV_FLAG_NO_SCHE_THREAD);
    set_state(co_env_state::idle);
    while (state() != co_env_state::destorying)
    {
        schedule_switch();
        set_state(co_env_state::idle); //  切换到idle协程，说明空闲了
        remove_detached_ctx__();       // 切换回来之后，将完成的ctx删除

        // 切回idle之后，睡眠等待
        std::unique_lock<std::recursive_mutex> lock(mu_wake_up_idle__);
        // CO_O_DEBUG("idle co start wait add ctx or destroying ... %p", this);
        if (state() == co_env_state::destorying)
        {
            break;
        }
        if (!scheduler__->can_schedule())
        {
            cond_wake_schedule__.wait(lock);
        }
    }

    // CO_O_DEBUG("stop schedule, prepare to cleanup");

    remove_all_ctx__();
    remove_current_env__();
}

void co_env::schedule_in_this_thread()
{
    start_schedule_routine__();
}

co_manager* co_env::manager() const
{
    return manager__;
}

void co_env::remove_all_ctx__()
{
    auto all_ctx = scheduler__->all_ctx();
    for (auto& ctx : all_ctx)
    {
        remove_ctx(ctx);
    }
}

void co_env::remove_current_env__()
{
    if (current_env__ != nullptr)
    {
        assert(manager__ != nullptr);
        // CO_O_DEBUG("add self to clean up list: %p", current_env__);
        manager__->remove_env(current_env__);
        current_env__ = nullptr;
    }
}

co_scheduler* co_env::scheduler() const
{
    return scheduler__;
}

bool co_env::scheduled() const
{
    return test_flag(CO_ENV_FLAG_SCHEDULED);
}

void co_env::reset_scheduled()
{
    reset_flag(CO_ENV_FLAG_SCHEDULED);
}

void co_env::lock_schedule()
{
    mu_schedule__.lock();
}

void co_env::unlock_schedule()
{
    mu_schedule__.unlock();
}

std::list<co_ctx*> co_env::moveable_ctx_list()
{
    auto               all_ctx = scheduler__->all_ctx();
    std::list<co_ctx*> ret;
    for (auto& ctx : all_ctx)
    {
        // 绑定env的协程和当前协程不能移动
        if (ctx->state() == co_state::running || ctx->test_flag(CO_CTX_FLAG_BIND) || ctx->test_flag(CO_CTX_FLAG_SHARED_STACK))
        {
            continue;
        }
        ret.push_back(ctx);
    }
    return ret;
}

void co_env::take_ctx(co_ctx* ctx)
{
    scheduler__->remove_ctx(ctx);
}

bool co_env::can_auto_destroy() const
{
    // 如果是用户自己转换的env，不能被选中销毁
    return !test_flag(CO_ENV_FLAG_COVERTED) && !test_flag(CO_ENV_FLAG_DONT_AUTO_DESTORY);
}

void co_env::wake_up()
{
    std::lock_guard<std::recursive_mutex> lock(mu_wake_up_idle__);
    // // CO_O_DEBUG("wake up env: %p", this);
    cond_wake_schedule__.notify_one();
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

void co_env::init_ctx(co_ctx* ctx)
{
    auto      regs  = ctx->regs();
    co_stack* stack = ctx->stack();
    if (ctx->test_flag(CO_CTX_FLAG_SHARED_STACK))
    {
        stack = shared_stack__;
    }
    auto config = ctx->config();
#ifdef _MSC_VER
#ifdef _WIN64
    regs[reg_index_RSP__] = stack->stack_top();
    regs[reg_index_RBP__] = regs[reg_index_RSP__];
    regs[reg_index_RIP__] = reinterpret_cast<co_byte*>(config.startup);
    regs[reg_index_RCX__] = reinterpret_cast<co_byte*>(ctx);

    __get_mxcsr_msvc_x64(&regs[reg_index_MXCSR__]);
    __get_fcw_msvc_x64(&regs[reg_index_FCW__]);
    __get_TEB_8_msvc_x64(&regs[reg_index_TEB_8__]);
    __get_TEB_16_msvc_x64(&regs[reg_index_TEB_16__]);
    __get_TEB_24_msvc_x64(&regs[reg_index_TEB_24__]);
    __get_TEB_5240_msvc_x64(&regs[reg_index_TEB_5240__]);

#else
#error only support x86_64 in msvc
#endif
#endif

#ifdef __GNUC__
#ifdef __x86_64__
    regs[reg_index_RSP__] = stack->stack_top();
    regs[reg_index_RBP__] = regs[reg_index_RSP__];
    regs[reg_index_RIP__] = reinterpret_cast<co_byte*>(co_entry);
    regs[reg_index_RDI__] = reinterpret_cast<co_byte*>(ctx);

    __get_mxcsr_gcc_x64(&regs[reg_index_MXCSR__]);
    __get_fcw_gcc_x64(&regs[reg_index_FCW__]);
#else
#error only supported x86_64
#endif
#endif
}

CO_NAMESPACE_END