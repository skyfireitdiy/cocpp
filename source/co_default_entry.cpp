#include "co_default_entry.h"
#include "co_ctx.h"
#include "co_define.h"
#include "co_env.h"
#include <any>
#include <functional>

void co_default_entry(co_ctx* ctx)
{
    ctx->config().entry(ctx->ret_ref());
    ctx->set_state(co_state::finished);
    ctx->env()->schedule_switch();
}