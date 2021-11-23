#include "co_scheduler.h"

CO_NAMESPACE_BEGIN

void co_schedulable_obj::set_scheduler_ctx(co_ctx* ctx)
{
    ctx__ = ctx;
}

co_ctx* co_schedulable_obj::scheduler_ctx() const
{
    return ctx__;
}

CO_NAMESPACE_END