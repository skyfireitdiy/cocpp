#include "co_sync_helper.h"
#include "co_ctx.h"
#include "co_spinlock.h"
#include "co_this_co.h"

CO_NAMESPACE_BEGIN

void lock_yield__(co_spinlock& lk, std::function<bool()> checker)
{
    do
    {
        lk.unlock();
        this_co::yield();
        lk.lock();
    } while (checker());
}

void ctx_enter_wait_state__(co_ctx* ctx, int rc_type, void* rc, std::deque<co_ctx*>& wait_deque)
{
    ctx_enter_wait_state__(ctx, rc_type, rc, wait_deque, ctx);
}

void wake_front__(std::deque<co_ctx*>& wait_deque)
{
    wake_front__(wait_deque, std::function([](co_ctx*& ctx) {
                     ctx->leave_wait_rc_state();
                 }));
}

CO_NAMESPACE_END