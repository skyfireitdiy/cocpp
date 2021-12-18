#include "co_env.h"
#include "co.h"
#include "co_ctx.h"
#include "co_ctx_config.h"
#include "co_ctx_factory.h"
#include "co_defer.h"
#include "co_define.h"
#include "co_env_factory.h"
#include "co_scheduler.h"
#include "co_stack.h"
#include "co_type.h"
#include "co_vos.h"

#include <cassert>
#include <cstring>
#include <future>
#include <iterator>

CO_NAMESPACE_BEGIN

thread_local co_env* current_env__ = nullptr;

co_env::co_env(co_scheduler* scheduler, co_stack* shared_stack, co_ctx* idle_ctx, bool create_new_thread)
    : sleep_controller__([this] { return need_sleep__(); })
    , scheduler__(scheduler)
    , shared_stack__(shared_stack)
    , idle_ctx__(idle_ctx)
{
    reset_safepoint();
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
    assert(state() != co_env_state::created && state() != co_env_state::destorying);
    init_ctx(shared_stack__, ctx); // 初始化ctx
    ctx_initted().pub(ctx);

    receive_ctx(ctx);

    // CO_O_DEBUG("add ctx wake up env: %p", this);
    ctx_added().pub(ctx);
}

void co_env::receive_ctx(co_ctx* ctx)
{
    assert(ctx != nullptr);
    assert(state() != co_env_state::created && state() != co_env_state::destorying);

    ctx->set_env(this);

    {
        std::lock_guard<co_spinlock> lock(scheduler__->mu_scheduleable_ctx__);
        scheduler__->all_scheduleable_ctx__[ctx->priority()].push_back(ctx);
        scheduler__->update_min_priority__(ctx->priority());
    }

    set_state(co_env_state::busy);

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
            wait_ctx_timeout().pub(ctx);
            return ret;
        }
        schedule_switch(false);
    }
    wait_ctx_finished().pub(ctx);
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
        schedule_switch(false);
        // CO_O_DEBUG("ctx %s %p state: %d", ctx->config().name.c_str(), ctx, ctx->state());
    }
    wait_ctx_finished().pub(ctx);
    return ctx->ret_ref();
}

int co_env::workload() const
{
    std::lock_guard<co_spinlock> lock(scheduler__->mu_scheduleable_ctx__);

    size_t ret = 0;
    for (unsigned int i = scheduler__->min_priority__; i < scheduler__->all_scheduleable_ctx__.size(); ++i)
    {
        ret += scheduler__->all_scheduleable_ctx__[i].size();
    }
    return ret;
}

void co_env::remove_detached_ctx__()
{
    auto curr    = scheduler__->current_ctx();
    auto all_ctx = all_scheduleable_ctx__();
    for (auto& ctx : all_ctx)
    {
        // 注意：此处不能删除当前的ctx，如果删除了，switch_to的当前上下文就没地方保存了
        if (ctx->state() == co_state::finished && ctx->can_destroy() && ctx != curr)
        {
            remove_ctx(ctx);
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
        auto next = choose_ctx__();
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
    // CO_O_DEBUG("from %p to %p", curr, next);
    next->set_state(co_state::running);
}

bool co_env::prepare_to_switch(co_ctx*& from, co_ctx*& to)
{

    co_ctx* curr = current_ctx();
    co_ctx* next = next_ctx__();

    assert(curr != nullptr);
    assert(next != nullptr);

    set_flag(CO_ENV_FLAG_SCHEDULED);

    update_ctx_state__(curr, next);
    if (curr == next)
    {
        return false;
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

        // CO_DEBUG("prepare from:%p to:%p", shared_stack_switch_context__.from, shared_stack_switch_context__.to);

        // 设置正在切换状态，防止被转移到其他env上，因为to已经是running状态了，所以不用担心他
        if (curr->test_flag(CO_CTX_FLAG_SHARED_STACK))
        {
            // CO_DEBUG("set ctx %p CO_CTX_FLAG_SWITCHING flag", curr);
            curr->set_flag(CO_CTX_FLAG_SWITCHING);
        }

        if (curr == idle_ctx__)
        {
            return false;
        }

        next = idle_ctx__;
        // CO_DEBUG("from %p to idle %p", curr, idle_ctx__);
    }

    from = curr;
    to   = next;

    return true;
}

void co_env::switch_normal_ctx__()
{
    co_ctx* curr = nullptr;
    co_ctx* next = nullptr;
    {
        remove_detached_ctx__();

        if (!prepare_to_switch(curr, next))
        {
            return;
        }
    }

    set_safepoint();
    switch_to(curr->regs(), next->regs());
    switched_to().pub(curr);
}

void co_env::schedule_switch(bool safe_return)
{
    reset_safepoint();
    if (shared_stack_switch_context__.need_switch)
    {
        switch_shared_stack_ctx__();
    }
    else
    {
        switch_normal_ctx__();
    }
    if (!safe_return)
    {
        reset_safepoint();
    }
}

void co_env::remove_ctx(co_ctx* ctx)
{
    {
        std::lock_guard<co_spinlock> lock(scheduler__->mu_scheduleable_ctx__);
        // CO_O_DEBUG("remove ctx %s %p , state: %d", ctx->config().name.c_str(), ctx, (int)ctx->state());
        // 此处不能断言 curr__ != ctx，因为在最后清理所有的ctx的时候，可以删除当前ctx
        scheduler__->all_scheduleable_ctx__[ctx->priority()].remove(ctx);
    }
    ctx_factory__->destroy_ctx(ctx);
    ctx_removed().pub(ctx);
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
    if (!test_flag(CO_ENV_FLAG_NO_SCHE_THREAD))
    {
        set_state(co_env_state::destorying);
    }

    wake_up();

    // CO_O_DEBUG("%p : stop schedule wake up idle co", this);
    schedule_stopped().pub();
}

void co_env::start_schedule()
{
    worker__ = std::async(std::launch::async, [this]() {
        current_env__ = this;
        start_schedule_routine__();
    });
}

void co_env::switch_shared_stack_ctx__()
{

    shared_stack_switch_context__.need_switch = false;

    // CO_O_DEBUG("switch from:%p to:%p", shared_stack_switch_context__.from, shared_stack_switch_context__.to);

    if (shared_stack_switch_context__.from->test_flag(CO_CTX_FLAG_SHARED_STACK))
    {
        // CO_O_DEBUG("save ctx %p stack", shared_stack_switch_context__.from);
        save_shared_stack__(shared_stack_switch_context__.from);
        // 保存栈完成后退出切换状态
        shared_stack_switch_context__.from->reset_flag(CO_CTX_FLAG_SWITCHING);
    }
    if (shared_stack_switch_context__.to->test_flag(CO_CTX_FLAG_SHARED_STACK))
    {
        // CO_O_DEBUG("restore ctx %p stack", shared_stack_switch_context__.from);
        restore_shared_stack__(shared_stack_switch_context__.to);
    }

    // CO_O_DEBUG("from idle %p to %p", idle_ctx__, shared_stack_switch_context__.to);
    // 切换到to

    set_safepoint();
    switch_to(idle_ctx__->regs(), shared_stack_switch_context__.to->regs());

    switched_to().pub(idle_ctx__);
}

void co_env::start_schedule_routine__()
{
    reset_safepoint();
    schedule_thread_tid__ = gettid();
    schedule_started().pub();
    reset_flag(CO_ENV_FLAG_NO_SCHE_THREAD);
    set_state(co_env_state::idle);
    while (state() != co_env_state::destorying)
    {
        // CO_O_DEBUG("dont need switch shared stack");
        schedule_switch(false);

        // 切换回来检测是否需要执行共享栈切换
        if (shared_stack_switch_context__.need_switch)
        {
            continue;
        }

        set_state(co_env_state::idle); //  切换到idle协程，说明空闲了
        remove_detached_ctx__();       // 切换回来之后，将完成的ctx删除

        sleep_if_need();

        // 切回idle之后，睡眠等待
        // CO_O_DEBUG("idle co start wait add ctx or destroying ... %p", this);
        if (state() == co_env_state::destorying)
        {
            break;
        }
    }

    // CO_O_DEBUG("stop schedule, prepare to cleanup");

    remove_all_ctx__();
    task_finished().pub();
    current_env__ = nullptr;
}

void co_env::schedule_in_this_thread()
{
    this_thread_converted_to_schedule_thread().pub(std::this_thread::get_id());
    start_schedule_routine__();
}

void co_env::remove_all_ctx__()
{
    auto all_ctx = all_ctx__();
    for (auto& obj : all_ctx)
    {
        remove_ctx(obj);
    }
    all_ctx_removed().pub();
}

co_scheduler* co_env::scheduler() const
{

    return scheduler__;
}

void co_env::reset_scheduled_flag()
{
    reset_flag(CO_ENV_FLAG_SCHEDULED);
    scheduled_flag_reset().pub();
}

bool co_env::can_auto_destroy() const
{

    // 如果是用户自己转换的env，不能被选中销毁
    return !test_flag(CO_ENV_FLAG_COVERTED) && !test_flag(CO_ENV_FLAG_DONT_AUTO_DESTORY);
}

size_t co_env::get_valid_stack_size__(co_ctx* ctx)
{
    return ctx->stack()->stack_top() - get_rsp(ctx);
}

void co_env::save_shared_stack__(co_ctx* ctx)
{

    auto stack_size = get_valid_stack_size__(ctx);
    // CO_O_DEBUG("ctx %p valid stack size is %lu", ctx, stack_size);
    auto tmp_stack = stack_factory__->create_stack(stack_size);
    memcpy(tmp_stack->stack(), get_rsp(ctx), stack_size);
    ctx->set_stack(tmp_stack);
    shared_stack_saved().pub(ctx);
}

void co_env::restore_shared_stack__(co_ctx* ctx)
{

    // 第一次调度的时候stack为nullptr
    if (ctx->stack() != nullptr)
    {
        memcpy(shared_stack__->stack_top() - ctx->stack()->stack_size(), ctx->stack()->stack(), ctx->stack()->stack_size());
        // 销毁原stack
        stack_factory__->destroy_stack(ctx->stack());
    }
    else
    {
        // CO_O_DEBUG("ctx %p  stack is nullptr", ctx);
    }
    // 设置为共享栈
    ctx->set_stack(shared_stack__);
    shared_stack_restored().pub(ctx);
}

co_tid co_env::schedule_thread_tid() const
{
    return schedule_thread_tid__;
}

bool co_env::can_schedule_ctx() const
{

    auto s = state();
    return s != co_env_state::blocked && s != co_env_state::destorying && !test_flag(CO_ENV_FLAG_NO_SCHE_THREAD);
}

bool co_env::is_blocked() const
{
    return state() == co_env_state::busy && !test_flag(CO_ENV_FLAG_SCHEDULED);
}

bool co_env::safepoint() const
{
    return safepoint__;
}

void co_env::set_safepoint()
{
    safepoint__ = true;
}

void co_env::reset_safepoint()
{
    safepoint__ = false;
}

bool co_env::need_sleep__()
{
    return !scheduler__->can_schedule() && state() != co_env_state::destorying;
}

co_ctx* co_env::choose_ctx__()
{
    std::lock_guard<co_spinlock> lock(scheduler__->mu_scheduleable_ctx__);
    for (unsigned int i = scheduler__->min_priority__; i < scheduler__->all_scheduleable_ctx__.size(); ++i)
    {
        for (auto& ctx : scheduler__->all_scheduleable_ctx__[i])
        {
            if (ctx->can_schedule())
            {
                auto ret = ctx;
                scheduler__->all_scheduleable_ctx__[i].remove(ctx);
                scheduler__->all_scheduleable_ctx__[i].push_back(ret);
                scheduler__->curr_obj__     = ret;
                scheduler__->min_priority__ = i;
                return ret;
            }
        }
    }
    scheduler__->curr_obj__ = nullptr;
    return nullptr;
}

std::list<co_ctx*> co_env::all_ctx__()
{
    std::scoped_lock   lock(scheduler__->mu_scheduleable_ctx__, scheduler__->mu_blocked_ctx__);
    std::list<co_ctx*> ret;
    for (auto& lst : scheduler__->all_scheduleable_ctx__)
    {
        ret.insert(ret.end(), lst.begin(), lst.end());
    }
    ret.insert(ret.begin(), scheduler__->blocked_ctx__.begin(), scheduler__->blocked_ctx__.end());
    return ret;
}

std::list<co_ctx*> co_env::all_scheduleable_ctx__() const
{
    std::lock_guard<co_spinlock> lock(scheduler__->mu_scheduleable_ctx__);
    std::list<co_ctx*>           ret;
    for (auto& lst : scheduler__->all_scheduleable_ctx__)
    {
        ret.insert(ret.end(), lst.begin(), lst.end());
    }
    return ret;
}

CO_NAMESPACE_END