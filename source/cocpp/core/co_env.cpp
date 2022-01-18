#include "cocpp/core/co_env.h"
#include "cocpp/core/co_ctx.h"
#include "cocpp/core/co_ctx_config.h"
#include "cocpp/core/co_define.h"
#include "cocpp/core/co_interrupt_closer.h"
#include "cocpp/core/co_stack.h"
#include "cocpp/core/co_type.h"
#include "cocpp/core/co_vos.h"
#include "cocpp/exception/co_error.h"
#include "cocpp/interface/co.h"
#include "cocpp/utils/co_defer.h"

#include <cassert>
#include <cstring>
#include <future>
#include <iterator>
#include <mutex>

CO_NAMESPACE_BEGIN

thread_local co_env* current_env__ = nullptr;

co_env::co_env(co_stack* shared_stack, co_ctx* idle_ctx, bool create_new_thread)
    : sleep_controller__([this] { return need_sleep__(); })
    , shared_stack__(shared_stack)
    , idle_ctx__(idle_ctx)
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
    if (state() == co_env_state::created || state() == co_env_state::destorying)
    {
        throw co_error("env state error");
    }
    init_ctx(shared_stack__, ctx); // 初始化ctx
    ctx_initted().pub(ctx);

    move_ctx_to_here(ctx);

    ctx_added().pub(ctx);
}

void co_env::move_ctx_to_here(co_ctx* ctx)
{
    assert(ctx != nullptr);
    assert(state() != co_env_state::created && state() != co_env_state::destorying);

    ctx->set_env(this);

    {
        std::scoped_lock lock(mu_normal_ctx__);
        all_normal_ctx__[ctx->priority()].push_back(ctx);
    }

    update_min_priority__(ctx->priority());
    set_state(co_env_state::busy);

    wake_up();
    ctx_received().pub(ctx);
}

std::optional<co_return_value> co_env::wait_ctx(co_ctx* ctx, const std::chrono::nanoseconds& timeout)
{
    // 反转优先级，防止高优先级ctx永久等待低优先级ctx
    auto old_priority = current_ctx()->priority();
    current_ctx()->set_priority(ctx->priority());
    CoDefer(current_ctx()->set_priority(old_priority));

    std::optional<co_return_value> ret;

    auto now = std::chrono::high_resolution_clock::now();
    while (ctx->state() != co_state::finished)
    {
        if (std::chrono::high_resolution_clock::now() - now > timeout)
        {
            wait_ctx_timeout().pub(ctx);
            return ret;
        }
        schedule_switch();
    }
    wait_ctx_finished().pub(ctx);
    return ctx->ret_ref();
}

co_return_value co_env::wait_ctx(co_ctx* ctx)
{

    auto old_priority = current_ctx()->priority();
    current_ctx()->set_priority(ctx->priority());

    CoDefer(current_ctx()->set_priority(old_priority));

    while (ctx->state() != co_state::finished)
    {
        schedule_switch();
    }
    wait_ctx_finished().pub(ctx);
    return ctx->ret_ref();
}

int co_env::workload() const
{
    std::scoped_lock lock(mu_normal_ctx__, mu_min_priority__);

    size_t ret = 0;
    for (unsigned int i = min_priority__; i < all_normal_ctx__.size(); ++i)
    {
        ret += all_normal_ctx__[i].size();
    }
    return ret;
}

void co_env::remove_detached_ctx__()
{
    auto curr    = current_ctx();
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
        auto next = choose_ctx_from_normal_list__();
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
    next->set_state(co_state::running);
}

bool co_env::prepare_to_switch(co_ctx*& from, co_ctx*& to)
{
    co_ctx* curr = current_ctx();
    co_ctx* next = next_ctx__();

    assert(curr != nullptr);
    assert(next != nullptr);

    set_flag(CO_ENV_FLAG_SCHEDULED);
    if (next != idle_ctx__)
    {
        set_state(co_env_state::busy);
    }
    else
    {
        sleep_if_need([this] { return idle_ctx__->test_flag(CO_CTX_FLAG_WAITING); });
    }

    if (curr == next)
    {
        return false;
    }
    else
    {
        update_ctx_state__(curr, next);
    }
    if (curr->test_flag(CO_CTX_FLAG_SHARED_STACK) || next->test_flag(CO_CTX_FLAG_SHARED_STACK))
    {
        shared_stack_switch_info__.from        = curr;
        shared_stack_switch_info__.to          = next;
        shared_stack_switch_info__.need_switch = true;

        // CO_DEBUG("prepare from:%p to:%p", shared_stack_switch_context__.from, shared_stack_switch_context__.to);

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

    if (!prepare_to_switch(curr, next))
    {
        return;
    }

    unlock_schedule();
    decreate_interrupt_lock_count_with_lock();
    switch_to(curr->regs(), next->regs());
    increate_interrupt_lock_count_with_lock();
    lock_schedule();
    switched_to().pub(curr);
}

void co_env::schedule_switch()
{
    lock_schedule();
    co_interrupt_closer interrupt_closer;
    remove_detached_ctx__();
    if (shared_stack_switch_info__.need_switch)
    {
        switch_shared_stack_ctx__();
    }
    else
    {
        switch_normal_ctx__();
    }
    unlock_schedule();
}

void co_env::remove_ctx(co_ctx* ctx)
{
    {
        std::scoped_lock lock(mu_normal_ctx__);
        // 此处不能断言 curr__ != ctx，因为在最后清理所有的ctx的时候，可以删除当前ctx
        all_normal_ctx__[ctx->priority()].remove(ctx);
    }
    co::destroy_ctx(ctx);
    ctx_removed().pub(ctx);
}

co_ctx* co_env::current_ctx() const
{
    std::scoped_lock lock(mu_curr_ctx__);
    if (curr_ctx__ == nullptr)
    {
        return idle_ctx__;
    }
    return curr_ctx__;
}

void co_env::stop_schedule()
{
    if (!test_flag(CO_ENV_FLAG_NO_SCHE_THREAD))
    {
        set_state(co_env_state::destorying);
    }

    wake_up();

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
    shared_stack_switch_info__.need_switch = false;

    if (shared_stack_switch_info__.from->test_flag(CO_CTX_FLAG_SHARED_STACK))
    {
        save_shared_stack__(shared_stack_switch_info__.from);
    }
    if (shared_stack_switch_info__.to->test_flag(CO_CTX_FLAG_SHARED_STACK))
    {
        restore_shared_stack__(shared_stack_switch_info__.to);
    }

    // 切换到to

    unlock_schedule();
    decreate_interrupt_lock_count_with_lock();
    switch_to(idle_ctx__->regs(), shared_stack_switch_info__.to->regs());
    increate_interrupt_lock_count_with_lock();
    lock_schedule();
    switched_to().pub(idle_ctx__);
}

void co_env::start_schedule_routine__()
{
    co_interrupt_closer interrupt_closer;
    schedule_thread_tid__ = gettid();
    schedule_started().pub();
    reset_flag(CO_ENV_FLAG_NO_SCHE_THREAD);
    set_state(co_env_state::idle);
    while (state() != co_env_state::destorying)
    {
        decreate_interrupt_lock_count_with_lock();
        schedule_switch();
        increate_interrupt_lock_count_with_lock();

        // 切换回来检测是否需要执行共享栈切换
        if (shared_stack_switch_info__.need_switch)
        {
            continue;
        }

        set_state(co_env_state::idle); //  切换到idle协程，说明空闲了

        sleep_if_need();
    }

    remove_all_ctx__();
    task_finished().pub();
    current_env__ = nullptr;
}

void co_env::schedule_in_this_thread()
{
    if (!test_flag(CO_ENV_FLAG_NO_SCHE_THREAD))
    {
        throw co_error("schedule_in_this_thread: already started");
    }
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

void co_env::reset_scheduled_flag()
{
    reset_flag(CO_ENV_FLAG_SCHEDULED);
    scheduled_flag_reset().pub();
}

bool co_env::can_auto_destroy() const
{
    // 如果是用户自己转换的env，不能被选中销毁
    return !test_flag(CO_ENV_FLAG_COVERTED) && !test_flag(CO_ENV_FLAG_DONT_AUTO_DESTORY) && !test_flag(CO_ENV_FLAG_NO_SCHE_THREAD);
}

size_t co_env::get_valid_stack_size__(co_ctx* ctx)
{
    return ctx->stack()->stack_top() - get_rsp(ctx);
}

void co_env::save_shared_stack__(co_ctx* ctx)
{

    auto stack_size = get_valid_stack_size__(ctx);
    auto tmp_stack  = co::create_stack(stack_size);
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
        co::destroy_stack(ctx->stack());
    }
    else
    {
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

void co_env::handle_priority_changed(int old, co_ctx* ctx)
{
    std::scoped_lock lock(mu_normal_ctx__);
    for (auto iter = all_normal_ctx__[old].begin(); iter != all_normal_ctx__[old].end(); ++iter)
    {
        if (*iter == ctx)
        {
            all_normal_ctx__[old].erase(iter);
            all_normal_ctx__[ctx->priority()].push_back(ctx);
            update_min_priority__(ctx->priority());
            return;
        }
    }
    assert(false);
}

bool co_env::can_schedule__() const
{
    std::scoped_lock lock(mu_normal_ctx__, mu_min_priority__);
    for (unsigned int i = min_priority__; i < all_normal_ctx__.size(); ++i)
    {
        for (auto& ctx : all_normal_ctx__[i])
        {
            if (ctx->can_schedule())
            {
                return true;
            }
        }
    }
    return false;
}

bool co_env::is_blocked() const
{
    return (state() == co_env_state::busy || state() == co_env_state::blocked) && !test_flag(CO_ENV_FLAG_SCHEDULED);
}

bool co_env::need_sleep__()
{
    return !can_schedule__() && state() != co_env_state::destorying;
}

co_ctx* co_env::choose_ctx_from_normal_list__()
{
    std::scoped_lock lock(mu_normal_ctx__, mu_curr_ctx__, mu_min_priority__);
    for (unsigned int i = min_priority__; i < all_normal_ctx__.size(); ++i)
    {
        for (auto& ctx : all_normal_ctx__[i])
        {
            if (ctx->can_schedule())
            {
                auto ret = ctx;
                all_normal_ctx__[i].remove(ctx);
                all_normal_ctx__[i].push_back(ret);
                min_priority__ = i;
                curr_ctx__     = ret;
                return ret;
            }
        }
    }
    curr_ctx__ = nullptr;
    return nullptr;
}

std::list<co_ctx*> co_env::all_ctx__()
{
    std::scoped_lock   lock(mu_normal_ctx__);
    std::list<co_ctx*> ret;
    for (auto& lst : all_normal_ctx__)
    {
        ret.insert(ret.end(), lst.begin(), lst.end());
    }
    return ret;
}

std::list<co_ctx*> co_env::all_scheduleable_ctx__() const
{
    std::scoped_lock   lock(mu_normal_ctx__);
    std::list<co_ctx*> ret;
    for (auto& lst : all_normal_ctx__)
    {
        ret.insert(ret.end(), lst.begin(), lst.end());
    }
    return ret;
}

void co_env::ctx_leave_wait_state(co_ctx* ctx)
{
    if (ctx == idle_ctx__)
    {
        return;
    }
    update_min_priority__(ctx->priority());
}

void co_env::ctx_enter_wait_state(co_ctx* ctx)
{
    if (ctx == idle_ctx__)
    {
        return;
    }
}

std::list<co_ctx*> co_env::take_all_movable_ctx()
{
    std::scoped_lock   lock(mu_normal_ctx__, mu_min_priority__, schedule_lock__);
    std::list<co_ctx*> ret;
    for (unsigned int i = min_priority__; i < all_normal_ctx__.size(); ++i)
    {
        auto backup = all_normal_ctx__[i];
        for (auto& ctx : backup)
        {
            if (ctx->can_move())
            {
                all_normal_ctx__[i].remove(ctx);
                ret.push_back(ctx);
            }
        }
    }
    all_moveable_ctx_taken().pub(ret);
    return ret;
}

co_ctx* co_env::take_one_movable_ctx()
{
    std::scoped_lock lock(mu_normal_ctx__, mu_min_priority__, schedule_lock__);
    for (unsigned int i = min_priority__; i < all_normal_ctx__.size(); ++i)
    {
        auto backup = all_normal_ctx__[i];
        for (auto& ctx : backup)
        {
            if (ctx->can_move())
            {
                all_normal_ctx__[i].remove(ctx);
                one_moveable_ctx_taken().pub(ctx);
                return ctx;
            }
        }
    }
    one_moveable_ctx_taken().pub(nullptr);
    return nullptr;
}

void co_env::update_min_priority__(int priority)
{
    std::scoped_lock lock(mu_min_priority__);
    if (priority < min_priority__)
    {
        min_priority__ = priority;
    }
}

void co_env::lock_schedule()
{
    schedule_lock__.lock();
    schedule_locked().pub();
}

void co_env::unlock_schedule()
{
    schedule_lock__.unlock();
    schedule_unlocked().pub();
}

void co_env::set_state(const co_env_state& state)
{
    std::scoped_lock lock(sleep_lock());
    state_manager__.set_state(state);
}

void co_env::set_exclusive()
{
    set_flag(CO_ENV_FLAG_EXCLUSIVE);
}

bool co_env::exclusive() const
{
    return test_flag(CO_ENV_FLAG_EXCLUSIVE);
}

CO_NAMESPACE_END