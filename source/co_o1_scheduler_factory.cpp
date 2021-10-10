#include "co_o1_scheduler_factory.h"
#include "co_o1_scheduler.h"

co_scheduler* co_o1_scheduler_factory::create_scheduler()
{
    return new co_o1_scheduler();
}

void co_o1_scheduler_factory::destroy_scheduler(co_scheduler* scheduler)
{
    delete scheduler;
}
