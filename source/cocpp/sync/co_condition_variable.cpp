#include "cocpp/sync/co_condition_variable.h"
#include "cocpp/core/co_env.h"
#include "cocpp/core/co_manager.h"

CO_NAMESPACE_BEGIN

void notify_all_at_co_exit(co_condition_variable& cond)
{
    auto ctx = co_manager::instance()->current_env()->current_ctx();
    ctx->finished().sub([&]() {
        cond.notify_all();
    });
}

CO_NAMESPACE_END