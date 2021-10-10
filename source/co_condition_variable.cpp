#include "co_condition_variable.h"

void notify_all_at_co_exit(co_condition_variable& cond)
{
    auto ctx = co::current_ctx();
    ctx->co_finished().register_callback([&](co_ctx*) {
        cond.notify_all();
    });
}