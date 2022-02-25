_Pragma("once");

#include "cocpp/comm/co_event.h"
#include "cocpp/core/co_define.h"
#include "cocpp/core/co_return_value.h"
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

private:
    co_flag_manager<CO_ENV_FLAG_MAX>                                                flag_manager__;
    co_state_manager<co_env_state, co_env_state::created, co_env_state::destorying> state_manager__;

    std::future<void> worker__;

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
    mutable std::recursive_mutex schedule_lock__;

    std::condition_variable_any cv_sleep__;

    void sleep_if_need__();
    void sleep_if_need__(std::function<bool()> checker);

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
    void                           wake_up();

    CoConstMemberMethodProxy(&state_manager__, state);
    CoConstMemberMethodProxy(&flag_manager__, test_flag);
    CoMemberMethodProxy(&flag_manager__, set_flag);
    CoMemberMethodProxy(&flag_manager__, reset_flag);

    friend class co_object_pool<co_env>;
    friend class co_env_factory;
};

extern thread_local co_env* current_env__;

CO_NAMESPACE_END