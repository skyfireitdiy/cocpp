#include "co_scheduler_factory.h"
#include "co_scheduler.h"

CO_NAMESPACE_BEGIN

co_scheduler* co_scheduler_factory::create_scheduler()
{
    return new co_scheduler();
}

void co_scheduler_factory::destroy_scheduler(co_scheduler* scheduler)
{
    delete scheduler;
}

CO_NAMESPACE_END