#pragma once

#include "co_env.h"
#include <atomic>
#include <bitset>
#include <future>
#include <shared_mutex>

class co_manager;

class co_default_env : public co_env
{
private:
    static constexpr int CO_ENV_FLAG_NO_SCHE_THREAD = 0;
    static constexpr int CO_ENV_FLAG_MAX_VALUE      = 8;

    co_stack*                          shared_stack__;
    std::future<void>                  worker__;
    std::bitset<CO_ENV_FLAG_MAX_VALUE> flag__;
    std::atomic<co_env_state>          state__;
    co_manager*                        manager__ = nullptr;

    // 所有ctx集合以及其对应的保护锁
    std::vector<co_ctx*>      all_ctx__;
    mutable std::shared_mutex mu_all_ctx__;

    size_t current_index__ = 0;

    co_ctx* idle_ctx__;

    co_default_env(co_ctx* idle_ctx, co_stack* shared_stack, bool create_new_thread);

    void        start_schedule_routine__();
    static void switch_to__(co_byte** curr_regs, co_byte** next_regs);

public:
    co_stack*             shared_stack() const override;
    void                  add_ctx(co_ctx* ctx) override;
    std::optional<co_ret> wait_ctx(co_ctx*&                         ctx,
                                   const std::chrono::milliseconds& timeout)
        override;
    co_ret       wait_ctx(co_ctx*& ctx) override;
    int          workload() const override;
    bool         has_scheduler_thread() const override;
    co_env_state state() const override;
    void         set_state(co_env_state state) override;
    void         schedule_switch() override;
    void         remove_ctx(co_ctx* ctx) override;
    co_ctx*      current_ctx() const override;
    co_ctx*      idle_ctx() const override;
    void         stop_schedule() override;
    void         start_schedule() override;
    void         schedule_in_this_thread() override;
    void         set_manager(co_manager* manager) override;
    co_manager*  manager() const override;

    friend class co_default_env_factory;
};