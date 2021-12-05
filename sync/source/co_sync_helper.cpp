#include "co_sync_helper.h"
#include "co_ctx.h"
#include "co_spinlock.h"
#include "co_this_co.h"

CO_NAMESPACE_BEGIN

void lock_yield__(co_ctx* ctx, co_spinlock& lk, std::function<bool()> checker)
{
    do
    {
        lk.unlock(ctx);
        this_co::yield();
        lk.lock(ctx);
    } while (checker());
}

void ctx_enter_wait_state__(co_ctx* ctx, int rc_type, void* rc, std::list<co_ctx*>& wait_list)
{
    ctx_enter_wait_state__(ctx, rc_type, rc, wait_list, ctx);
}

void wake_front__(std::list<co_ctx*>& wait_list)
{
    wake_front__(wait_list, std::function([](co_ctx*& ctx) {
                     ctx->leave_wait_rc_state();
                 }));
}

CO_NAMESPACE_END