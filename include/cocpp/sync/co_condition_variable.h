_Pragma("once");

#include "cocpp/core/co_define.h"
#include "cocpp/core/co_env.h"
#include "cocpp/core/co_manager.h"
#include "cocpp/core/co_timer.h"
#include "cocpp/core/co_type.h"
#include "cocpp/sync/co_spinlock.h"
#include "cocpp/utils/co_noncopyable.h"

#include <chrono>
#include <list>
#include <mutex>

CO_NAMESPACE_BEGIN

class co_ctx;
class co_condition_variable;
void notify_all_at_co_exit(co_condition_variable& cond);

class co_condition_variable final : private co_noncopyable
{
private:
    co_spinlock        cv_lock__;
    std::list<co_ctx*> waiters__;
    bool               timeout__ { false };

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
void co_condition_variable::wait(Lock& lock)
{
    auto ctx = co_manager::instance()->current_env()->current_ctx();
    ctx->enter_wait_resource_state(co_waited_rc_type::condition_variable, this);
    {
        CO_O_DEBUG("co_condition_variable::wait: ctx: %p\n", ctx);
        std::scoped_lock lk(cv_lock__);
        waiters__.push_back(ctx);
    }
    lock.unlock();
    co_manager::instance()->current_env()->schedule_switch();
    lock.lock();
}

template <typename Lock, typename Predicate>
void co_condition_variable::wait(Lock& lock, Predicate pred)
{
    auto ctx = co_manager::instance()->current_env()->current_ctx();
    lock.unlock();
    do
    {
        ctx->enter_wait_resource_state(co_waited_rc_type::condition_variable, this);
        {
            CO_O_DEBUG("co_condition_variable::wait with pred: ctx: %p\n", ctx);
            std::scoped_lock lk(cv_lock__);
            waiters__.push_back(ctx);
        }
        co_manager::instance()->current_env()->schedule_switch();
    } while (!pred());
    lock.lock();
}

template <typename Lock, typename Clock, typename Duration>
bool co_condition_variable::wait_until(Lock& lock, const std::chrono::time_point<Clock, Duration>& abs_time)
{
    co_manager::instance()->current_env()->lock_schedule();
    auto ctx = co_manager::instance()->current_env()->current_ctx();

    ctx->enter_wait_resource_state(co_waited_rc_type::condition_variable, this);
    {
        CO_O_DEBUG("co_condition_variable::wait_until ctx: %p\n", ctx);
        std::scoped_lock lk(cv_lock__);
        waiters__.push_back(ctx);
    }

    auto timer = co_timer::create([this, ctx] {
        {
            std::scoped_lock lk(cv_lock__);
            waiters__.remove(ctx);
        }
        timeout__ = true;
        ctx->leave_wait_resource_state();
    },
                                  abs_time);
    timer->start();
    co_manager::instance()->current_env()->unlock_schedule();
    lock.unlock();
    co_manager::instance()->current_env()->schedule_switch();
    lock.lock();
    timer->stop();
    return !timeout__;
}

template <typename Lock, typename Clock, typename Duration>
bool co_condition_variable::wait_for(Lock& lock, const std::chrono::duration<Clock, Duration>& duration)
{
    return wait_until(lock, std::chrono::steady_clock::now() + duration);
}

template <typename Lock, typename Clock, typename Duration, typename Predicate>
bool co_condition_variable::wait_until(Lock& lock, const std::chrono::time_point<Clock, Duration>& abs_time,
                                       Predicate pred)
{
    co_manager::instance()->current_env()->lock_schedule();
    auto ctx   = co_manager::instance()->current_env()->current_ctx();
    auto timer = co_timer::create([this, ctx] {
        {
            std::scoped_lock lk(cv_lock__);
            waiters__.remove(ctx);
        }
        timeout__ = true;
        ctx->leave_wait_resource_state();
    },
                                  abs_time);
    timer->start();

    lock.unlock();
    co_manager::instance()->current_env()->unlock_schedule();
    do
    {
        ctx->enter_wait_resource_state(co_waited_rc_type::condition_variable, this);
        {
            CO_O_DEBUG("co_condition_variable::wait_until with pred: ctx: %p\n", ctx);
            std::scoped_lock lk(cv_lock__);
            waiters__.push_back(ctx);
        }
        co_manager::instance()->current_env()->schedule_switch();
    } while (!timeout__ && !pred());

    lock.lock();
    timer->stop();

    return !timeout__ && pred();
}

template <typename Lock, typename Clock, typename Duration, typename Predicate>
bool co_condition_variable::wait_for(Lock& lock, const std::chrono::duration<Clock, Duration>& duration,
                                     Predicate pred)
{
    return wait_until(lock, std::chrono::steady_clock::now() + duration, pred);
}

CO_NAMESPACE_END