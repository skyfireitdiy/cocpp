#include "co_default_entry.h"
#include "co_ctx.h"
#include "co_define.h"
#include "co_env.h"
#include <any>
#include <cassert>
#include <functional>

void co_default_entry(co_ctx* ctx)
{
    ctx->config().entry(ctx->ret_ref());
    CO_DEBUG("ctx %s %p finished\n", ctx->config().name.c_str(), ctx);
    ctx->set_state(co_state::finished);
    assert(ctx->env() != nullptr);
    ctx->env()->schedule_switch(); // 此处的ctx对应的env不可能为空，如果为空，这个ctx就不可能被调度
}