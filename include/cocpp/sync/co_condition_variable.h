_Pragma("once");

#include "cocpp/core/co_ctx.h"
#include "cocpp/core/co_define.h"
#include "cocpp/core/co_env.h"
#include "cocpp/core/co_manager.h"
#include "cocpp/core/co_timer.h"
#include "cocpp/core/co_type.h"
#include "cocpp/sync/co_mutex.h"
#include "cocpp/utils/co_noncopyable.h"

#include <chrono>
#include <list>
#include <mutex>

CO_NAMESPACE_BEGIN

class co_ctx;
class co_condition_variable_impl;

// using co_condition_variable = std::condition_variable_any;
using co_condition_variable = co_condition_variable_impl;

void notify_all_at_co_exit(co_condition_variable& cond);

class co_condition_variable_impl final : private co_noncopyable
{
private:
    mutable co_spinlock cv_lock__;
    std::list<co_ctx*>  waiters__;
    bool                timeout__ { false };

public:
    template <typename Lock>
    void wait(Lock& lock);
    template <typename Lock, typename Predicate>
    void wait(Lock& lock, Predicate pred);

    void notify_all();
    void notify_one();

    template <typename Lock, typename Clock, typename Duration>
    bool wait_until(Lock& lock, const std::chrono::time_point<Clock, Duration>& abs_time);
    template <typename Lock, typename Clock, typename Duration>
    bool wait_for(Lock& lock, const std::chrono::duration<Clock, Duration>& duration);

    template <typename Lock, typename Clock, typename Duration, typename Predicate>
    bool wait_until(Lock& lock, const std::chrono::time_point<Clock, Duration>& abs_time, Predicate pred);
    template <typename Lock, typename Clock, typename Duration, typename Predicate>
    bool wait_for(Lock& lock, const std::chrono::duration<Clock, Duration>& duration, Predicate pred);
};

template <typename Lock>
void co_condition_variable_impl::wait(Lock& lock)
{
    CoPreemptGuard();
    auto ctx = CoCurrentCtx();
    {
        std::scoped_lock lk(cv_lock__);

        waiters__.push_back(ctx);
        lock.unlock();
        ctx->enter_wait_resource_state(this);
    }
    CoYield();
    lock.lock();
}

template <typename Lock, typename Predicate>
void co_condition_variable_impl::wait(Lock& lock, Predicate pred)
{
    CoPreemptGuard();
    auto ctx = CoCurrentCtx();

    do
    {
        {
            std::scoped_lock lk(cv_lock__);
            waiters__.push_back(ctx);
            lock.unlock();
            ctx->enter_wait_resource_state(this);
        }

        CoYield();

        lock.lock();
    } while (!pred());
}

template <typename Lock, typename Clock, typename Duration>
bool co_condition_variable_impl::wait_until(Lock& lock, const std::chrono::time_point<Clock, Duration>& abs_time)
{
    CoScheduleGuard();
    auto ctx = CoCurrentCtx();

    auto timer = co_timer::create([this, ctx] {
        std::scoped_lock lk(cv_lock__);
        waiters__.remove(ctx);
        timeout__ = true;
        ctx->leave_wait_resource_state(this);
    },
                                  abs_time);
    timer->start();
    {
        std::scoped_lock lk(cv_lock__);

        waiters__.push_back(ctx);
        lock.unlock();
        ctx->enter_wait_resource_state(this);
    }
    CoUnlockSchedule();
    CoYield();
    CoLockSchedule();
    lock.lock();
    timer->stop();
    return !timeout__;
}

template <typename Lock, typename Clock, typename Duration>
bool co_condition_variable_impl::wait_for(Lock& lock, const std::chrono::duration<Clock, Duration>& duration)
{
    return wait_until(lock, std::chrono::steady_clock::now() + duration);
}

template <typename Lock, typename Clock, typename Duration, typename Predicate>
bool co_condition_variable_impl::wait_until(Lock& lock, const std::chrono::time_point<Clock, Duration>& abs_time,
                                            Predicate pred)
{
    CoScheduleGuard();
    auto ctx   = CoCurrentCtx();
    auto timer = co_timer::create([this, ctx] {
        std::scoped_lock lk(cv_lock__);
        waiters__.remove(ctx);
        timeout__ = true;
        ctx->leave_wait_resource_state(this);
    },
                                  abs_time);
    timer->start();
    do
    {
        {
            std::scoped_lock lk(cv_lock__);

            waiters__.push_back(ctx);
            lock.unlock();
            ctx->enter_wait_resource_state(this);
        }
        CoUnlockSchedule();
        CoYield();
        CoLockSchedule();
        lock.lock();
    } while (!timeout__ && !pred());

    timer->stop();

    return !timeout__ && pred();
}

template <typename Lock, typename Clock, typename Duration, typename Predicate>
bool co_condition_variable_impl::wait_for(Lock& lock, const std::chrono::duration<Clock, Duration>& duration,
                                          Predicate pred)
{
    return wait_until(lock, std::chrono::steady_clock::now() + duration, pred);
}

CO_NAMESPACE_END