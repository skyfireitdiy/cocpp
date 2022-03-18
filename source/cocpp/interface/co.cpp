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
    auto ctx = ctx__;
    ctx__    = nullptr;
    ctx->unlock_destroy();
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
    std::exception_ptr e;
    try
    {
        wait<void>();
    }
    catch (...)
    {
        e = std::current_exception();
    }
    if (ctx__ != nullptr)
    {
        auto ctx = ctx__;
        ctx__    = nullptr;
        ctx->unlock_destroy();
    }
    if (e)
    {
        std::rethrow_exception(e);
    }
}

co::co(co&& other) noexcept
{
    ctx__       = other.ctx__;
    other.ctx__ = nullptr;
}

co& co::operator=(co&& other) noexcept
{
    if (this != &other)
    {
        join();
        ctx__       = other.ctx__;
        other.ctx__ = nullptr;
    }
    return *this;
}

namespace this_co
{

void yield()
{
    co::current_env()->schedule_switch();
}

co_id id()
{
    return reinterpret_cast<co_id>(co::current_env()->current_ctx());
}

std::string name()
{
    return co::current_env()->current_ctx()->config().name;
}

}

CO_NAMESPACE_END
