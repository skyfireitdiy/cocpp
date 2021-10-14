#include "co.h"
#include "co_env_factory.h"
#include "co_stack_factory.h"
#include <cassert>
#include <thread>

CO_NAMESPACE_BEGIN

co_manager* co::manager__ = co_manager::instance();

void co::detach()
{
    if (ctx__ == nullptr)
    {
        return;
    }
    ctx__->unlock_destroy();
    ctx__ = nullptr;
}

std::string co::name() const
{
    if (ctx__ == nullptr)
    {
        return "";
    }
    return ctx__->config().name;
}

void co::convert_to_schedule_thread()
{
    manager__->current_env()->schedule_in_this_thread();
}

co::~co()
{
    detach();
}

co_id co::id() const
{
    return reinterpret_cast<co_id>(ctx__);
}

co_ctx* co::current_ctx()
{
    return manager__->current_env()->current_ctx();
}

void co::set_custom_scheduler_factory(co_scheduler_factory* scheduler_factory)
{
    co_env_factory::instance()->set_scheduler_factory(scheduler_factory);
}

co_env* co::current_env()
{
    return manager__->current_env();
}

co_env* co::create_env()
{
    return manager__->create_env(true);
}

CO_NAMESPACE_END
