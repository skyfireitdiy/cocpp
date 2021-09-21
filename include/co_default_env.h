#pragma once

#include "co_env.h"
#include <atomic>
#include <bitset>
#include <chrono>
#include <future>
#include <mutex>
#include <shared_mutex>

class co_manager;
class co_scheduler;

class co_default_env : public co_env
{
private:
    static constexpr int CO_ENV_FLAG_NO_SCHE_THREAD = 0;
    static constexpr int CO_ENV_FLAG_MAX_VALUE      = 8;

    co_stack*                          shared_stack__;
    std::future<void>                  worker__;
    std::bitset<CO_ENV_FLAG_MAX_VALUE> flag__;

    co_env_state              state__;
    mutable std::shared_mutex mu_state__;

    co_manager* manager__ = nullptr;

    co_scheduler* scheduler__ = nullptr;

    co_ctx* idle_ctx__;

    mutable std::mutex      mu_wake_up_idle__;
    std::condition_variable cond_wake_schedule__;

    std::atomic<bool> scheduled__ { true };

    std::mutex mu_schedule__;

    co_default_env(co_scheduler* scheduler, co_ctx* idle_ctx, co_stack* shared_stack, bool create_new_thread);

    void        start_schedule_routine__();
    void        remove_detached_ctx__();
    void        remove_all_ctx__();
    void        remove_current_env__();
    co_ctx*     next_ctx__();
    void        update_ctx_state__(co_ctx* curr, co_ctx* next);
    static void switch_to__(co_byte** curr_regs, co_byte** next_regs);

public:
    co_stack*             shared_stack() const override;
    void                  add_ctx(co_ctx* ctx) override;
    std::optional<co_ret> wait_ctx(co_ctx*                         ctx,
                                   const std::chrono::nanoseconds& timeout)
        override;
    co_ret             wait_ctx(co_ctx* ctx) override;
    int                workload() const override;
    bool               has_scheduler_thread() const override;
    co_env_state       state() const override;
    void               set_state(co_env_state state) override;
    void               schedule_switch() override;
    void               remove_ctx(co_ctx* ctx) override;
    co_ctx*            current_ctx() const override;
    co_ctx*            idle_ctx() const override;
    void               stop_schedule() override;
    void               start_schedule() override;
    void               schedule_in_this_thread() override;
    void               set_manager(co_manager* manager) override;
    co_manager*        manager() const override;
    co_scheduler*      scheduler() const override;
    bool               scheduled() const override;
    void               reset_scheduled() override;
    void               lock_schedule() override;
    void               unlock_schedule() override;
    std::list<co_ctx*> moveable_ctx_list() override;
    void               take_ctx(co_ctx* ctx) override;

    friend class co_default_env_factory;
};