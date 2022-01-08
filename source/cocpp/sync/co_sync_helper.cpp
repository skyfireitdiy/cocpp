#include "cocpp/sync/co_sync_helper.h"
#include "cocpp/core/co_ctx.h"
#include "cocpp/interface/co_this_co.h"
#include "cocpp/sync/co_spinlock.h"

CO_NAMESPACE_BEGIN


void ctx_enter_wait_state__(co_ctx* ctx, int rc_type, void* rc, std::deque<co_ctx*>& wait_deque)
{
    ctx_enter_wait_state__(ctx, rc_type, rc, wait_deque, ctx);
}

void wake_front__(std::deque<co_ctx*>& wait_deque)
{
    wake_front__(wait_deque, std::function([](co_ctx*& ctx) {
                     ctx->leave_wait_resource_state();
                 }));
}

CO_NAMESPACE_END