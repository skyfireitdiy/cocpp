#include "cocpp/core/co_env.h"
#include "cocpp/core/co_ctx.h"
#include "cocpp/core/co_ctx_config.h"
#include "cocpp/core/co_ctx_factory.h"
#include "cocpp/core/co_define.h"
#include "cocpp/core/co_manager.h"
#include "cocpp/core/co_stack.h"
#include "cocpp/core/co_stack_factory.h"
#include "cocpp/core/co_timer.h"
#include "cocpp/core/co_type.h"
#include "cocpp/core/co_vos.h"
#include "cocpp/utils/co_defer.h"

#include <algorithm>
#include <bits/ranges_algo.h>
#include <cassert>
#include <chrono>
#include <cstring>
#include <future>
#include <iterator>
#include <mutex>
#include <ranges>
#include <sstream>
#include <stdexcept>

using namespace std;

CO_NAMESPACE_BEGIN

thread_local co_env *current_env__ = nullptr;

co_env::co_env(size_t shared_stack_size, co_ctx *idle_ctx, bool create_new_thread)
    : shared_stack_size__(shared_stack_size)
    , idle_ctx__(idle_ctx)
{
    // Note: this value may be changed by start_schedule()
    schedule_thread_tid__ = gettid();
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
        set_flag(CO_ENV_FLAG_CONVERTED);
        current_env__ = this;
    }
}

void co_env::create_shared_stack__()
{
    shared_stack__ = co_stack_factory::create_stack(shared_stack_size__);
}

void co_env::add_ctx(co_ctx *ctx)
{
    assert(ctx != nullptr);
    if (state() == co_env_state::created || state() == co_env_state::destroying)
    {
        throw runtime_error("env state error");
    }

    auto is_shared_stack = ctx->test_flag(CO_CTX_FLAG_SHARED_STACK);

    if (is_shared_stack)
    {
        call_once(shared_stack_once_flag__, [this] {
            create_shared_stack__();
        });
    }

    init_ctx(is_shared_stack ? shared_stack__ : ctx->stack(), ctx); // 初始化ctx

    move_ctx_to_here(ctx);
}

void co_env::move_ctx_to_here(co_ctx *ctx)
{
    assert(ctx != nullptr);
    assert(state() != co_env_state::created && state() != co_env_state::destroying);

    ctx->set_env(this);

    {
        scoped_lock lock(mu_normal_ctx__);
        all_normal_ctx__[ctx->priority()].push_back(ctx);
        ++ctx_count__;
    }

    update_min_priority__(ctx->priority());
    set_state(co_env_state::busy);

    wake_up();
}

optional<co_return_value> co_env::wait_ctx(co_ctx *ctx, const chrono::nanoseconds &timeout)
{
    // Priority inversion prevents high priorities from being kept waiting and low priorities from being executed
    auto old_priority = current_ctx()->priority();
    current_ctx()->set_priority(ctx->priority());
    CoDefer(current_ctx()->set_priority(old_priority));

    optional<co_return_value> ret;

    ctx->lock_finished_state();
    lock_schedule();
    auto curr_ctx = current_ctx();
    if (ctx->state() != co_state::finished)
    {
        auto timer = co_timer::create(
            nullptr,
            co_expire_type::once, chrono::duration_cast<chrono::milliseconds>(timeout).count());
        auto timer_id = timer.get();
        curr_ctx->enter_wait_resource_state(timer_id);
        auto handle = ctx->finished().sub([curr_ctx, timer, timer_id] {
            timer->stop(); // Handle delete callbacks first and wake up later to prevent timing problems
            curr_ctx->leave_wait_resource_state(timer_id);
        });
        timer->set_expire_callback([curr_ctx, ctx, handle, timer_id] {
            ctx->finished().unsub(handle); // Unsubscribe before wake up to prevent timing problems
            curr_ctx->leave_wait_resource_state(timer_id);
        });
        timer->start();

        unlock_schedule();
        ctx->unlock_finished_state();
        schedule_switch();
    }
    else
    {
        unlock_schedule();
        ctx->unlock_finished_state();
    }

    ctx->lock_finished_state();
    if (ctx->state() != co_state::finished)
    {
        ctx->unlock_finished_state();
        return ret;
    }
    ctx->unlock_finished_state();
    ctx->check_and_rethrow_exception();
    return ctx->ret_ref();
}

co_return_value co_env::wait_ctx(co_ctx *ctx)
{

    auto old_priority = current_ctx()->priority();
    current_ctx()->set_priority(ctx->priority());

    CoDefer(current_ctx()->set_priority(old_priority));

    ctx->lock_finished_state();
    lock_schedule();
    auto curr_ctx = current_ctx();
    if (ctx->state() != co_state::finished)
    {
        curr_ctx->enter_wait_resource_state(ctx);
        ctx->finished().sub([curr_ctx, ctx] {
            curr_ctx->leave_wait_resource_state(ctx);
        });
        unlock_schedule();
        ctx->unlock_finished_state();
        schedule_switch();
    }
    else
    {
        unlock_schedule();
        ctx->unlock_finished_state();
    }

    ctx->check_and_rethrow_exception();
    return ctx->ret_ref();
}

size_t co_env::workload() const
{
    scoped_lock lock(mu_normal_ctx__);
    return ctx_count__;
}

size_t co_env::ctx_count() const
{
    scoped_lock lock(mu_normal_ctx__);
    return ctx_count__;
}

void co_env::remove_detached_ctx__()
{
    auto curr = current_ctx();
    auto all_ctx = all_ctx__();

    ranges::for_each(
        all_ctx
            | views::filter([curr](co_ctx *ctx) {
                  return ctx->state() == co_state::finished && ctx->can_destroy() && ctx != curr;
              }),
        [this](co_ctx *ctx) {
            remove_ctx(ctx);
        });
}

co_ctx *co_env::next_ctx__()
{
    // 如果要销毁、并且可以销毁，切换到idle销毁
    if (state() == co_env_state::destroying)
    {
        return idle_ctx__;
    }
    else
    {
        auto next = choose_ctx_from_normal_list__();
        return next == nullptr ? idle_ctx__ : next;
    }
}

void co_env::update_ctx_state__(co_ctx *curr, co_ctx *next)
{
    // 如果当前运行的ctx已经完成，状态不变
    if (curr->state() != co_state::finished)
    {
        curr->set_state(co_state::suspended);
    }
    next->set_state(co_state::running);
}

// 此函数调用前必须保证调度已经被锁定
bool co_env::prepare_to_switch(co_ctx *&from, co_ctx *&to)
{
    co_ctx *curr = current_ctx();
    co_ctx *next = next_ctx__();

    assert(curr != nullptr);
    assert(next != nullptr);

    set_flag(CO_ENV_FLAG_SCHEDULED);
    if (next != idle_ctx__)
    {
        set_state(co_env_state::busy);
    }
    else
    {
        // The scheduling lock needs to be unlocked before going to sleep, otherwise other threads may be blocked
        unlock_schedule();
        // Wake up when the current cooperative journey is not blocked (The current coroutine is the idle coroutine)
        // The purpose of need_sleep__ is to prevent new coroutines from joining during sleep
        sleep_if_need__([curr, this] {
            return need_sleep__() && curr->test_flag(CO_CTX_FLAG_WAITING);
        });
        lock_schedule();
    }

    // no need to switch
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
        shared_stack_switch_info__.from = curr;
        shared_stack_switch_info__.to = next;
        shared_stack_switch_info__.need_switch = true;

        if (curr == idle_ctx__)
        {
            return false;
        }

        next = idle_ctx__;
    }

    from = curr;
    to = next;

    return true;
}

void co_env::switch_normal_ctx__()
{
    co_ctx *curr = nullptr;
    co_ctx *next = nullptr;

    if (!prepare_to_switch(curr, next))
    {
        return;
    }

    unlock_schedule();
    switch_to(curr->regs(), next->regs());
    lock_schedule();
}

void co_env::schedule_switch()
{
    lock_schedule();
    CoDefer(unlock_schedule());
    remove_detached_ctx__();
    if (shared_stack_switch_info__.need_switch)
    {
        switch_shared_stack_ctx__();
    }
    else
    {
        switch_normal_ctx__();
    }
    if (current_ctx()->pre_ctx() != nullptr)
    {
        current_ctx()->pre_ctx()->shrink_stack();
        current_ctx()->clear_pre_ctx();
    }
}

void co_env::remove_ctx(co_ctx *ctx)
{
    {
        scoped_lock lock(mu_normal_ctx__);
        // 此处不能断言 curr__ != ctx，因为在最后清理所有的ctx的时候，可以删除当前ctx
        all_normal_ctx__[ctx->priority()].remove(ctx);
        --ctx_count__;
    }
    co_ctx_factory::destroy_ctx(ctx);
}

// Warning: The CTX returned by this interface is not guaranteed to be valid after the call.
co_ctx *co_env::current_ctx() const
{
    auto ret = curr_ctx__.load();
    return ret != nullptr ? ret : idle_ctx__;
}

void co_env::stop_schedule()
{
    if (!test_flag(CO_ENV_FLAG_NO_SCHE_THREAD))
    {
        set_state(co_env_state::destroying);
    }

    wake_up();
}

void co_env::start_schedule()
{
    worker__ = async(launch::async, [this]() {
        current_env__ = this;
        start_schedule_routine__();
    });
}

void co_env::switch_shared_stack_ctx__()
{
    CoPreemptGuard();
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
    switch_to(idle_ctx__->regs(), shared_stack_switch_info__.to->regs());
    lock_schedule();
}

void co_env::start_schedule_routine__()
{
    schedule_thread_tid__ = gettid();
    unset_flag(CO_ENV_FLAG_NO_SCHE_THREAD);
    set_state(co_env_state::idle);
    {
        scoped_lock lock(schedule_lock__);
        while (state() != co_env_state::destroying)
        {
            unlock_schedule();
            schedule_switch();
            lock_schedule();

            // Switch back to check if a shared stack switch needs to be performed
            if (shared_stack_switch_info__.need_switch)
            {
                continue;
            }

            // Switch to the idle coroutine, indicating that it is idle
            set_state(co_env_state::idle);
            unlock_schedule();
            sleep_if_need__();
            lock_schedule();
        }
    }

    remove_all_ctx__();
    task_finished().pub();
    current_env__ = nullptr;
}

void co_env::schedule_in_this_thread()
{
    if (!test_flag(CO_ENV_FLAG_NO_SCHE_THREAD))
    {
        throw runtime_error("schedule_in_this_thread: already started");
    }
    start_schedule_routine__();
}

void co_env::remove_all_ctx__()
{
    ranges::for_each(all_ctx__(), [this](auto &ctx) {
        remove_ctx(ctx);
    });
}

void co_env::reset_scheduled_flag()
{
    unset_flag(CO_ENV_FLAG_SCHEDULED);
}

bool co_env::can_auto_destroy() const
{
    // 如果是用户自己转换的env，不能被选中销毁
    return !(test_flag(CO_ENV_FLAG_CONVERTED) || test_flag(CO_ENV_FLAG_DONT_AUTO_DESTROY) || test_flag(CO_ENV_FLAG_NO_SCHE_THREAD));
}

size_t co_env::get_valid_stack_size__(co_ctx *ctx)
{
    return ctx->stack()->stack_top() - get_rsp(ctx);
}

void co_env::save_shared_stack__(co_ctx *ctx)
{
    auto stack_size = get_valid_stack_size__(ctx);
    auto tmp_stack = co_stack_factory::create_stack(stack_size);
    memcpy(tmp_stack->stack(), get_rsp(ctx), stack_size);
    ctx->set_stack(tmp_stack);
}

void co_env::restore_shared_stack__(co_ctx *ctx)
{

    // 第一次调度的时候stack为nullptr
    if (ctx->stack() != nullptr)
    {
        memcpy(shared_stack__->stack_top() - ctx->stack()->stack_size(), ctx->stack()->stack(), ctx->stack()->stack_size());
        // 销毁原stack
        co_stack_factory::destroy_stack(ctx->stack());
    }
    else
    {
    }
    // 设置为共享栈
    ctx->set_stack(shared_stack__);
}

co_tid co_env::schedule_thread_tid() const
{
    return schedule_thread_tid__;
}

bool co_env::can_schedule_ctx() const
{

    auto s = state();
    return s != co_env_state::blocked && s != co_env_state::destroying && !test_flag(CO_ENV_FLAG_NO_SCHE_THREAD);
}

void co_env::handle_priority_changed(int old, co_ctx *ctx)
{
    scoped_lock lock(mu_normal_ctx__);

    auto target = ranges::find_if(all_normal_ctx__[old], [ctx](auto &iter) {
        return iter == ctx;
    });

    all_normal_ctx__[old].erase(target);
    all_normal_ctx__[ctx->priority()].push_back(ctx);
    update_min_priority__(ctx->priority());
}

bool co_env::can_schedule__() const
{
    scoped_lock lock(mu_normal_ctx__, mu_min_priority__);

    return ranges::any_of(views::iota(min_priority__, all_normal_ctx__.size()),
                          [this](const auto &i) -> bool {
                              return ranges::any_of(all_normal_ctx__[i], [](auto &ctx) -> bool {
                                  return ctx->can_schedule();
                              });
                          });
}

bool co_env::is_blocked() const
{
    return (state() == co_env_state::busy || state() == co_env_state::blocked) && !test_flag(CO_ENV_FLAG_SCHEDULED);
}

bool co_env::need_sleep__()
{
    // Hibernation requires the following conditions:
    // 1. No schedulable coroutines
    // 2. The destroy flag is not set
    return !can_schedule__() && state() != co_env_state::destroying;
}

co_ctx *co_env::choose_ctx_from_normal_list__()
{
    scoped_lock lock(mu_normal_ctx__, mu_min_priority__);

    for (unsigned int i = min_priority__; i < all_normal_ctx__.size(); ++i)
    {
        for (auto &&ctx : all_normal_ctx__[i])
        {
            if (ctx->can_schedule())
            {
                auto ret = ctx;
                all_normal_ctx__[i].remove(ctx);
                all_normal_ctx__[i].push_back(ret);
                min_priority__ = i;
                curr_ctx__ = ret;
                return ret;
            }
        }
    }
    curr_ctx__ = nullptr;
    return nullptr;
}

list<co_ctx *> co_env::all_ctx__()
{
    CoPreemptGuard();
    scoped_lock lock(mu_normal_ctx__);
    auto ret = all_normal_ctx__ | views::join;
    return {ret.begin(), ret.end()};
}

void co_env::ctx_leave_wait_state(co_ctx *ctx)
{
    if (ctx == idle_ctx__)
    {
        return;
    }
    update_min_priority__(ctx->priority());
}

void co_env::ctx_enter_wait_state(co_ctx *ctx)
{
    if (ctx == idle_ctx__)
    {
        return;
    }
}

list<co_ctx *> co_env::take_all_movable_ctx()
{
    scoped_lock lock(mu_normal_ctx__, mu_min_priority__, schedule_lock__);
    list<co_ctx *> ret;
    for (unsigned int i = min_priority__; i < all_normal_ctx__.size(); ++i)
    {
        auto backup = all_normal_ctx__[i];
        for (auto &&ctx : backup)
        {
            if (ctx->can_move())
            {
                all_normal_ctx__[i].remove(ctx);
                --ctx_count__;
                ret.push_back(ctx);
            }
        }
    }
    return ret;
}

co_ctx *co_env::take_one_movable_ctx()
{
    scoped_lock lock(mu_normal_ctx__, mu_min_priority__, schedule_lock__);
    for (unsigned int i = min_priority__; i < all_normal_ctx__.size(); ++i)
    {
        auto backup = all_normal_ctx__[i];
        for (auto &&ctx : backup)
        {
            if (ctx->can_move())
            {
                all_normal_ctx__[i].remove(ctx);
                --ctx_count__;
                return ctx;
            }
        }
    }
    return nullptr;
}

void co_env::update_min_priority__(size_t priority)
{
    scoped_lock lock(mu_min_priority__);
    if (priority < min_priority__)
    {
        min_priority__ = priority;
    }
}

bool co_env::has_ctx() const
{
    scoped_lock lock(mu_normal_ctx__);
    return ctx_count__ > 0;
}

void co_env::lock_schedule()
{
    schedule_lock__.lock();
}

void co_env::unlock_schedule()
{
    schedule_lock__.unlock();
}

bool co_env::try_lock_schedule()
{
    return schedule_lock__.try_lock();
}

void co_env::set_state(const co_env_state &state)
{
    scoped_lock lock(schedule_lock__);
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

void co_env::wake_up()
{
    CoPreemptGuard();
    scoped_lock lock(schedule_lock__);
    cv_sleep__.notify_one();
}

void co_env::sleep_if_need__()
{
    CoPreemptGuard();
    unique_lock lock(schedule_lock__);
    while (need_sleep__())
    {
        cv_sleep__.wait(lock);
    }
}

void co_env::sleep_if_need__(function<bool()> checker)
{
    CoPreemptGuard();
    unique_lock lock(schedule_lock__);
    while (checker())
    {
        cv_sleep__.wait(lock);
    }
}

bool co_env::can_force_schedule() const
{
    return schedule_lock__.count() == 0;
}

co_recursive_mutex_with_count &co_env::schedule_lock()
{
    return schedule_lock__;
}

std::string co_env::env_info()
{
    scoped_lock lock(schedule_lock__, mu_normal_ctx__, mu_min_priority__);

    stringstream ss;

    ss << "------------ env start -----------" << endl;
    ss << "env:" << this << endl;
    // flags
    ss << "flags: \n";
    for (auto i = 0; i < CO_ENV_FLAG_MAX; ++i)
    {
        ss << i << "=" << (test_flag(i) ? "true" : "false") << endl;
    }

    // state
    ss << "state: " << static_cast<int>(state_manager__.state()) << endl;

    // shared stack
    if (shared_stack__)
    {
        ss << "shared stack: \n"
           << endl;
        ss << shared_stack__->stack_info() << endl;
    }
    else
    {
        ss << "shared stack: null" << endl;
    }

    // shared stack size
    ss << "shared stack size: " << shared_stack_size__ << endl;

    // idle ctx
    if (idle_ctx__)
    {
        ss << "idle ctx:" << endl;
        ss << idle_ctx__->ctx_info();
    }
    else
    {
        ss << "idle ctx: null" << endl;
    }

    // schedule thread tid
    ss << "schedule thread tid: " << schedule_thread_tid__ << endl;

    // min priority
    ss << "min priority: " << min_priority__ << endl;

    // normal ctx
    ss << "ctx count:" << ctx_count__ << endl;
    ss << "current ctx: " << curr_ctx__ << endl;
    ss << "normal ctx: " << endl;
    for (size_t i = 0; i < all_normal_ctx__.size(); ++i)
    {
        if (all_normal_ctx__[i].empty())
        {
            continue;
        }
        ss << "priority " << i << ": " << endl;
        for (auto &&ctx : all_normal_ctx__[i])
        {

            ss << ctx->ctx_info() << endl;
        }
    }

    ss << "------------ env end -----------" << endl;

    return ss.str();
}

void co_recursive_mutex_with_count::lock()
{
    ++count__;
    assert(count__ > 0);
    recursive_mutex::lock();
}

void co_recursive_mutex_with_count::unlock()
{
    recursive_mutex::unlock();
    --count__;
    assert(count__ >= 0);
}

bool co_recursive_mutex_with_count::try_lock()
{
    ++count__;
    if (recursive_mutex::try_lock())
    {
        assert(count__ > 0);
        return true;
    }
    --count__;
    return false;
}

long long int co_recursive_mutex_with_count::count() const
{
    return count__;
}

void co_recursive_mutex_with_count::increate_count()
{
    ++count__;
    assert(count__ > 0);
}

void co_recursive_mutex_with_count::decreate_count()
{
    --count__;
    assert(count__ >= 0);
}

co_schedule_guard::co_schedule_guard(co_recursive_mutex_with_count &lk)
    : lk__(lk)
{
    lk__.lock();
}

co_schedule_guard::~co_schedule_guard()
{
    lk__.unlock();
}

co_preempt_guard::co_preempt_guard(co_recursive_mutex_with_count &lk)
    : lk__(lk)
{
    lk__.increate_count();
}

co_preempt_guard::~co_preempt_guard()
{
    lk__.decreate_count();
}

CO_NAMESPACE_END
