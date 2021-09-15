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

co::co_env_destoryer::~co_env_destoryer()
{
    if (current_env__ != nullptr)
    {
        co::manager__->env_factory()->destroy_env(current_env__);
        current_env__ = nullptr;
    }
}

void co::uninit_co()
{
    co::manager__->clean_up();
}

void co::detach()
{
    ctx__->set_detach();
}

thread_local co::co_env_destoryer env_destoryer__;
