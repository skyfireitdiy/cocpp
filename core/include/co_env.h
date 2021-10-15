#pragma once

#include "co_ctx_factory.h"
#include "co_define.h"
#include "co_event.h"
#include "co_flag_manager.h"
#include "co_manager.h"
#include "co_nocopy.h"
#include "co_return_value.h"
#include "co_stack.h"
#include "co_stack_factory.h"
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
                     public co_flag_manager<CO_ENV_FLAG_MAX>
{

    RegCoEvent(env_task_finished);

private:
    std::future<void> worker__;

    mutable std::shared_mutex mu_state__;

    co_ctx_factory* const   ctx_factory__ { co_ctx_factory::instance() };
    co_stack_factory* const stack_factory__ { co_stack_factory::instance() };

    co_scheduler* const scheduler__ = nullptr;
    co_stack*           shared_stack__ { nullptr };
    co_ctx* const       idle_ctx__ { nullptr };

    co_env_state state__ { co_env_state::idle };

    mutable std::recursive_mutex mu_wake_up_idle__;
    std::condition_variable_any  cond_wake_schedule__;

    std::mutex mu_schedule__;

    co_env(co_scheduler* scheduler, co_stack* shared_stack, co_ctx* idle_ctx, bool create_new_thread);

    void               start_schedule_routine__();
    void               remove_detached_ctx__();
    void               remove_all_ctx__();
    void               remove_current_env__();
    co_ctx*            next_ctx__();
    void               update_ctx_state__(co_ctx* curr, co_ctx* next);
    void               save_shared_stack__(co_ctx* ctx);
    void               restore_shared_stack__(co_ctx* ctx);
    void               switch_shared_stack_ctx__();
    void               lock_schedule__();
    void               unlock_schedule__();
    std::list<co_ctx*> moveable_ctx_list__();
    void               take_ctx__(co_ctx* ctx);

    static void   switch_to__(co_byte** curr_regs, co_byte** next_regs);
    static size_t get_valid_stack_size(co_ctx* ctx);

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

    struct
    {
        co_ctx* from { nullptr };
        co_ctx* to { nullptr };
        bool    need_switch { false };
    } shared_stack_switch_context__;

public:
    void                           init_ctx(co_ctx* ctx);
    void                           add_ctx(co_ctx* ctx);
    std::optional<co_return_value> wait_ctx(co_ctx*                         ctx,
                                            const std::chrono::nanoseconds& timeout);
    co_return_value                wait_ctx(co_ctx* ctx);
    int                            workload() const;
    co_env_state                   state() const;
    void                           set_state(co_env_state state);
    void                           schedule_switch();
    void                           remove_ctx(co_ctx* ctx);
    co_ctx*                        current_ctx() const;
    void                           stop_schedule();
    void                           start_schedule();
    void                           schedule_in_this_thread();
    co_scheduler*                  scheduler() const;
    bool                           scheduled() const;
    void                           reset_scheduled_flag();
    std::list<co_ctx*>             take_moveable_ctx();
    bool                           can_auto_destroy() const;
    void                           wake_up();

    friend class co_env_factory;
};

extern thread_local co_env* current_env__;

CO_NAMESPACE_END