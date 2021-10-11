#pragma once

#include "co_ctx_factory.h"
#include "co_define.h"
#include "co_flag_manager.h"
#include "co_manager.h"
#include "co_nocopy.h"
#include "co_return_value.h"
#include "co_type.h"

#include <atomic>
#include <bitset>
#include <chrono>
#include <condition_variable>
#include <future>
#include <list>
#include <mutex>
#include <optional>
#include <shared_mutex>

CO_NAMESPACE_BEGIN

class co_manager;
class co_scheduler;
class co_ctx;

class co_env final : public co_nocopy,
                     public co_flag_manager<CO_ENV_FLAG_MAX_VALUE>
{
private:
    std::future<void> worker__;

    mutable std::shared_mutex mu_state__;

    co_manager* const     manager__ { co_manager::instance() };
    co_ctx_factory* const ctx_factory__ { co_ctx_factory::instance() };
    co_scheduler* const   scheduler__ = nullptr;
    co_ctx* const         idle_ctx__;

    co_env_state state__;

    mutable std::recursive_mutex mu_wake_up_idle__;
    std::condition_variable_any  cond_wake_schedule__;

    std::mutex mu_schedule__;

    co_env(co_scheduler* scheduler, co_ctx* idle_ctx, bool create_new_thread);

    void        start_schedule_routine__();
    void        remove_detached_ctx__();
    void        remove_all_ctx__();
    void        remove_current_env__();
    co_ctx*     next_ctx__();
    void        update_ctx_state__(co_ctx* curr, co_ctx* next);
    static void switch_to__(co_byte** curr_regs, co_byte** next_regs);

    static constexpr int reg_index_RDI__   = 0;
    static constexpr int reg_index_RIP__   = 1;
    static constexpr int reg_index_RSP__   = 2;
    static constexpr int reg_index_RBP__   = 3;
    static constexpr int reg_index_RBX__   = 4;
    static constexpr int reg_index_R12__   = 5;
    static constexpr int reg_index_R13__   = 6;
    static constexpr int reg_index_R14__   = 7;
    static constexpr int reg_index_R15__   = 8;
    static constexpr int reg_index_MXCSR__ = 9;
    static constexpr int reg_index_FCW__   = 10;

public:
    void                           init_ctx(co_ctx* ctx);
    void                           add_ctx(co_ctx* ctx);
    std::optional<co_return_value> wait_ctx(co_ctx*                         ctx,
                                            const std::chrono::nanoseconds& timeout);
    co_return_value                wait_ctx(co_ctx* ctx);
    int                            workload() const;
    bool                           has_scheduler_thread() const;
    co_env_state                   state() const;
    void                           set_state(co_env_state state);
    void                           schedule_switch();
    void                           remove_ctx(co_ctx* ctx);
    co_ctx*                        current_ctx() const;
    co_ctx*                        idle_ctx() const;
    void                           stop_schedule();
    void                           start_schedule();
    void                           schedule_in_this_thread();
    co_manager*                    manager() const;
    co_scheduler*                  scheduler() const;
    bool                           scheduled() const;
    void                           reset_scheduled();
    void                           lock_schedule();
    void                           unlock_schedule();
    std::list<co_ctx*>             moveable_ctx_list();
    void                           take_ctx(co_ctx* ctx);
    bool                           can_auto_destroy() const;
    void                           wake_up();

    friend class co_env_factory;
};

extern thread_local co_env* current_env__;

CO_NAMESPACE_END