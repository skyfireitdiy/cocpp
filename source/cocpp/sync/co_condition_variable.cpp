#include "cocpp/sync/co_condition_variable.h"
#include "cocpp/core/co_define.h"
#include "cocpp/core/co_env.h"
#include "cocpp/core/co_manager.h"

CO_NAMESPACE_BEGIN

void notify_all_at_co_exit(co_condition_variable& cond)
{
    auto ctx = CoCurrentCtx();
    ctx->finished().sub([&]() {
        cond.notify_all();
    });
}

void co_condition_variable_impl::notify_all()
{
    CoPreemptGuard();
    std::scoped_lock lk(cv_lock__);
    for (auto ctx : waiters__)
    {
        ctx->leave_wait_resource_state(this);
    }
    waiters__.clear();
}

void co_condition_variable_impl::notify_one()
{
    CoPreemptGuard();
    std::scoped_lock lk(cv_lock__);
    if (waiters__.empty())
    {
        return;
    }
    auto ctx = waiters__.front();
    waiters__.pop_front();
    ctx->leave_wait_resource_state(this);
}

co_condition_variable_impl::co_condition_variable_impl(co_condition_variable_impl&& other)
    : cv_lock__(std::move(other.cv_lock__))
    , waiters__(std::move(other.waiters__))
    , timeout__(std::move(other.timeout__))
{
    other.timeout__ = false;
}

co_condition_variable_impl& co_condition_variable_impl::operator=(co_condition_variable_impl&& other)
{
    cv_lock__       = std::move(other.cv_lock__);
    waiters__       = std::move(other.waiters__);
    timeout__       = std::move(other.timeout__);
    other.timeout__ = false;
    return *this;
}

CO_NAMESPACE_END