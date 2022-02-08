#include "cocpp/interface/co.h"
#include "cocpp/core/co_env_factory.h"
#include "cocpp/core/co_stack_factory.h"
#include <cassert>
#include <thread>

CO_NAMESPACE_BEGIN

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
    join();
}

co_id co::id() const
{
    return reinterpret_cast<co_id>(ctx__);
}

void co::join()
{
    wait<void>();
}

CO_NAMESPACE_END
