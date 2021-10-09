#include "co.h"
#include "co_env_factory.h"
#include <cassert>
#include <thread>

co_manager* co::manager__ = nullptr;

void co::init_co(co_manager* manager)
{
    co::manager__ = manager;
}

void co::schedule_switch()
{
    co::manager__->current_env()->schedule_switch();
}

void co::uninit_co()
{
    co::manager__->set_clean_up();
}

void co::detach()
{
    if (ctx__ == nullptr)
    {
        return;
    }
    if (co::manager__->clean_up()) // manager已经设置了clean_up，所有的ctx都会被销毁
    {
        return;
    }
    ctx__->unlock_destroy();
    ctx__ = nullptr;
}

co_id co::this_co::id()
{
    return reinterpret_cast<co_id>(co::manager__->current_env()->current_ctx());
}

std::string co::name() const
{
    if (ctx__ == nullptr)
    {
        return "";
    }
    return ctx__->config().name;
}

std::string co::this_co::name()
{
    return co::manager__->current_env()->current_ctx()->config().name;
}

void co::convert_to_schedule_thread()
{
    co::manager__->current_env()->schedule_in_this_thread();
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