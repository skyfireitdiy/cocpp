#pragma once

#include "co_define.h"
#include "co_entry.h"
#include "co_scheduler.h"

#include <list>
#include <mutex>
#include <vector>

class co_o1_scheduler : public co_scheduler
{
private:
    std::vector<std::list<co_ctx*>> all_ctx__;
    mutable std::mutex              mu_all_ctx__;
    co_ctx*                         curr__ = nullptr;

    static bool ctx_can_schedule__(co_ctx* ctx);

    co_o1_scheduler();

public:
    void               add_ctx(co_ctx* ctx) override;
    void               remove_ctx(co_ctx* ctx) override;
    co_ctx*            choose_ctx() override;
    std::list<co_ctx*> all_ctx() const override;
    size_t             count() const override;
    co_ctx*            current_ctx() const override;
    bool               can_schedule() const override;

    friend class co_o1_scheduler_factory;
};