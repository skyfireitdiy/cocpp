_Pragma("once");

#include "cocpp/comm/co_event.h"
#include "cocpp/core/co_define.h"
#include "cocpp/core/co_return_value.h"
#include "cocpp/core/co_sleep_controller.h"
#include "cocpp/core/co_type.h"
#include "cocpp/mem/co_object_pool.h"
#include "cocpp/utils/co_flag_manager.h"
#include "cocpp/utils/co_noncopyable.h"
#include "cocpp/utils/co_state_manager.h"

#include <chrono>
#include <future>
#include <list>
#include <mutex>
#include <optional>
#include <thread>

CO_NAMESPACE_BEGIN

class co_ctx;
class co_stack;

class co_env final : private co_noncopyable
{
    RegCoEvent(task_finished);
    RegCoEvent(ctx_added, co_ctx*);
    RegCoEvent(ctx_received, co_ctx*);
    RegCoEvent(wait_ctx_timeout, co_ctx*);
    RegCoEvent(wait_ctx_finished, co_ctx*);
    RegCoEvent(state_changed, co_env_state, co_env_state);
    RegCoEvent(switched_to, co_ctx*);
    RegCoEvent(ctx_removed, co_ctx*);
    RegCoEvent(schedule_stopped);
    RegCoEvent(schedule_started);
    RegCoEvent(idle_waited);
    RegCoEvent(idle_waked);
    RegCoEvent(all_ctx_removed);
    RegCoEvent(scheduled_flag_reset);
    RegCoEvent(schedule_locked);
    RegCoEvent(schedule_unlocked);
    RegCoEvent(ctx_taken, co_ctx*);
    RegCoEvent(ctx_initted, co_ctx*);
    RegCoEvent(shared_stack_saved, co_ctx*);
    RegCoEvent(shared_stack_restored, co_ctx*);
    RegCoEvent(all_moveable_ctx_taken, std::list<co_ctx*>);
    RegCoEvent(one_moveable_ctx_taken, co_ctx*);
    RegCoEvent(this_thread_converted_to_schedule_thread, std::thread::id);

private:
    co_flag_manager<CO_ENV_FLAG_MAX>                                                flag_manager__;
    co_state_manager<co_env_state, co_env_state::created, co_env_state::destorying> state_manager__;

    std::future<void> worker__;

    co_sleep_controller sleep_controller__;

    co_stack*      shared_stack__ { nullptr };
    size_t         shared_stack_size__ { 0 };
    std::once_flag shared_stack_once_flag__;

    co_ctx* const idle_ctx__ { nullptr };

    co_tid schedule_thread_tid__ {};

    // Another thread will add a new CTX to this list.
    std::vector<std::list<co_ctx*>> all_normal_ctx__ { CO_MAX_PRIORITY };
    mutable std::recursive_mutex    mu_normal_ctx__;
    size_t                          ctx_count__ { 0 };

    // Another thread will get current CTX from this ENV.
    std::atomic<co_ctx*> curr_ctx__ { nullptr };

    int                          min_priority__ = 0;
    mutable std::recursive_mutex mu_min_priority__;
    std::mutex                   schedule_lock__;

    co_env(size_t shared_stack_size, co_ctx* idle_ctx, bool create_new_thread);
    void               start_schedule_routine__();
    void               remove_detached_ctx__();
    void               remove_all_ctx__();
    co_ctx*            next_ctx__();
    void               save_shared_stack__(co_ctx* ctx);
    void               restore_shared_stack__(co_ctx* ctx);
    void               switch_shared_stack_ctx__();
    void               switch_normal_ctx__();
    bool               need_sleep__();
    co_ctx*            choose_ctx_from_normal_list__();
    std::list<co_ctx*> all_ctx__();
    bool               can_schedule__() const;
    void               update_min_priority__(int priority);
    void               create_shared_stack__();
    static size_t      get_valid_stack_size__(co_ctx* ctx);
    static void        update_ctx_state__(co_ctx* curr, co_ctx* next);
    struct
    {
        co_ctx* from { nullptr };
        co_ctx* to { nullptr };
        bool    need_switch { false };
    } shared_stack_switch_info__;

public:
    void                           add_ctx(co_ctx* ctx);
    void                           move_ctx_to_here(co_ctx* ctx);
    std::optional<co_return_value> wait_ctx(co_ctx* ctx, const std::chrono::nanoseconds& timeout);
    co_return_value                wait_ctx(co_ctx* ctx);
    size_t                         workload() const;
    void                           schedule_switch();
    void                           remove_ctx(co_ctx* ctx);
    co_ctx*                        current_ctx() const;
    void                           stop_schedule();
    void                           start_schedule();
    void                           schedule_in_this_thread();
    void                           reset_scheduled_flag();
    bool                           can_auto_destroy() const;
    co_tid                         schedule_thread_tid() const;
    bool                           can_schedule_ctx() const;
    bool                           is_blocked() const;
    bool                           prepare_to_switch(co_ctx*& from, co_ctx*& to);
    void                           handle_priority_changed(int old, co_ctx* ctx);
    void                           ctx_leave_wait_state(co_ctx* ctx);
    void                           ctx_enter_wait_state(co_ctx* ctx);
    std::list<co_ctx*>             take_all_movable_ctx();
    co_ctx*                        take_one_movable_ctx();
    void                           lock_schedule();
    void                           unlock_schedule();
    bool                           try_lock_schedule();
    void                           set_state(const co_env_state& state);
    void                           set_exclusive();
    bool                           exclusive() const;
    bool                           has_ctx() const;
    size_t                         ctx_count() const;

    CoConstMemberMethodProxy(&state_manager__, state);
    CoConstMemberMethodProxy(&flag_manager__, test_flag);
    CoMemberMethodProxy(&flag_manager__, set_flag);
    CoMemberMethodProxy(&flag_manager__, reset_flag);
    CoMemberMethodProxy(&sleep_controller__, sleep_if_need);
    CoMemberMethodProxy(&sleep_controller__, wake_up);
    CoMemberMethodProxy(&sleep_controller__, sleep_lock);

    friend class co_object_pool<co_env>;
    friend class co_env_factory;
};

extern thread_local co_env* current_env__;

CO_NAMESPACE_END