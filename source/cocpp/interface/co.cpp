#include "cocpp/interface/co.h"
#include "cocpp/core/co_env_factory.h"
#include "cocpp/core/co_stack_factory.h"
#include <cassert>
#include <thread>

CO_NAMESPACE_BEGIN

void co::detach()
{
    std::unique_lock lock(mu__, std::defer_lock);
    if (!lock.try_lock())
    {
        return;
    }
    if (ctx__ == nullptr)
    {
        return;
    }
    ctx__->unlock_destroy();
    ctx__ = nullptr;
}

std::string co::name() const
{
    std::unique_lock lock(mu__, std::defer_lock);
    if (!lock.try_lock())
    {
        return "";
    }
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
    std::unique_lock lock(mu__, std::defer_lock);
    if (!lock.try_lock())
    {
        return co_id();
    }
    return reinterpret_cast<co_id>(ctx__);
}

void co::join()
{
    std::unique_lock lock(mu__, std::defer_lock);
    if (!lock.try_lock())
    {
        return;
    }
    if (ctx__ == nullptr)
    {
        return;
    }
    auto ctx = ctx__;
    ctx->unlock_destroy();
    ctx__ = nullptr;
    manager__->current_env()->wait_ctx(ctx);
}

bool co::joinable() const
{
    std::unique_lock lock(mu__, std::defer_lock);
    if (!lock.try_lock())
    {
        return false;
    }
    return ctx__ != nullptr;
}

CO_NAMESPACE_END
