#include "co_spinlock.h"
#include "co.h"
#include "co_define.h"
#include "co_this_co.h"

CO_NAMESPACE_BEGIN

void co_spinlock::lock()
{
    auto    ctx  = co::current_ctx();
    co_ctx* null = nullptr;
    while (!owner__.compare_exchange_strong(null, ctx))
    {
        co::current_env()->schedule_switch(true);
        null = nullptr;
    }
}

void co_spinlock::unlock()
{
    auto    ctx  = co::current_ctx();
    co_ctx* curr = ctx;
    while (!owner__.compare_exchange_strong(curr, nullptr))
    {
        co::current_env()->schedule_switch(true);
        curr = ctx;
    }
}

bool co_spinlock::try_lock()
{
    auto    ctx  = co::current_ctx();
    co_ctx* null = nullptr;
    if (owner__.compare_exchange_strong(null, ctx))
    {
        return true;
    }
    else
    {
        return false;
    }
}

CO_NAMESPACE_END