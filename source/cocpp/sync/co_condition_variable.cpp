#include "cocpp/sync/co_condition_variable.h"
#include "cocpp/core/co_env.h"
#include "cocpp/core/co_manager.h"

CO_NAMESPACE_BEGIN

void notify_all_at_co_exit(co_condition_variable& cond)
{
    auto ctx = co_manager::instance()->current_env()->current_ctx();
    ctx->finished().sub([&]() {
        cond.notify_all();
    });
}

void co_condition_variable::notify_all()
{
    std::scoped_lock lk(cv_lock__);
    for (auto ctx : waiters__)
    {
        ctx->leave_wait_resource_state();
    }
    waiters__.clear();
}

void co_condition_variable::notify_one()
{
    std::scoped_lock lk(cv_lock__);
    if (waiters__.empty())
    {
        cv_lock__.unlock();
        return;
    }
    auto ctx = waiters__.front();
    waiters__.pop_front();
    ctx->leave_wait_resource_state();
}

CO_NAMESPACE_END