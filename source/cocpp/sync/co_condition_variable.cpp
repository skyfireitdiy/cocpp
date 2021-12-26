#include "cocpp/sync/co_condition_variable.h"

CO_NAMESPACE_BEGIN

void notify_all_at_co_exit(co_condition_variable& cond)
{
    auto ctx = co::current_ctx();
    ctx->finished().sub([&]() {
        cond.notify_all();
    });
}

CO_NAMESPACE_END