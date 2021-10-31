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
    return ctx__->name();
}

co::~co()
{
    detach();
}

co_id co::id() const
{
    return reinterpret_cast<co_id>(ctx__);
}
CO_NAMESPACE_END
