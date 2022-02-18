_Pragma("once");

#include "cocpp/comm/co_event.h"
#include "cocpp/core/co_ctx_factory.h"
#include "cocpp/core/co_define.h"
#include "cocpp/core/co_env_factory.h"
#include "cocpp/core/co_stack_factory.h"
#include "cocpp/utils/co_noncopyable.h"
#include "cocpp/utils/co_singleton.h"

#include <functional>
#include <future>
#include <list>
#include <mutex>
#include <set>

CO_NAMESPACE_BEGIN

class co_env;
class co_ctx;
class co_env_factory;
class co_ctx_factory;
class co_scheduler_factory;
class co_timer;
struct co_ctx_config;

using namespace std::chrono_literals;
class co_manager final : public co_singleton_static<co_manager>
{
    RegCoEvent(best_env_got, co_env*);
    RegCoEvent(env_created, co_env*);
    RegCoEvent(env_shared_stack_size_set, size_t);
    RegCoEvent(background_task_created);
    RegCoEvent(env_removed, co_env*);
    RegCoEvent(env_from_this_thread_created, co_env*);
    RegCoEvent(clean_up_set);
    RegCoEvent(env_routine_cleaned);
    RegCoEvent(base_thread_count_set, size_t);
    RegCoEvent(max_thread_count_set, size_t);
    RegCoEvent(ctx_redistributed);
    RegCoEvent(redundant_env_destroyed);
    RegCoEvent(timing_routine_finished);
    RegCoEvent(timing_duration_set);
    RegCoEvent(all_factory_destroyed);
    RegCoEvent(background_task_finished);
    RegCoEvent(ctx_created, co_ctx*);
    RegCoEvent(timing_routine_timout);

private:
    co_env_set env_set__ {
        .normal_env_count = 0,
        .base_env_count   = std::thread::hardware_concurrency(),
        .max_env_count    = std::thread::hardware_concurrency() * 2
    };

    bool                                clean_up__ { false };
    std::recursive_mutex                clean_up_lock__;
    std::list<std::future<void>>        background_task__;
    mutable std::recursive_mutex        mu_timer_duration__;
    std::chrono::steady_clock::duration timer_duration__ { 10ms };
    size_t                              default_shared_stack_size__ = CO_DEFAULT_STACK_SIZE;
    std::function<bool()>               need_free_mem_cb__ { [] { return false; } };
    std::recursive_mutex                need_free_mem_cb_lock__;
    std::set<std::shared_ptr<co_timer>> timer_queue__;
    std::recursive_mutex                mu_timer_queue__;
    std::condition_variable_any         cv_timer_queue__;

    void clean_env_routine__();
    void monitor_routine__();
    void timer_routine__();

    void    redistribute_ctx__();
    void    force_schedule__();
    void    destroy_redundant_env__();
    void    free_mem__();
    void    wait_background_task__();
    void    set_clean_up__();
    void    destroy_all_factory__();
    co_env* get_best_env__();
    void    remove_env__(co_env* env);
    void    subscribe_env_event__(co_env* env);
    void    subscribe_ctx_event__(co_ctx* env);
    void    subscribe_manager_event__();
    void    create_background_task__();
    void    create_env_from_this_thread__();
    void    steal_ctx_routine__();

    void insert_timer_to_queue__(std::shared_ptr<co_timer> timer);
    void remove_timer_from_queue__(std::shared_ptr<co_timer> timer);

    co_manager();

public:
    co_env* create_env(bool dont_auto_destory);
    co_ctx* create_and_schedule_ctx(
        const co_ctx_config&         config,
        std::function<void(co_any&)> entry,
        bool                         lock_destroy = true);

    void    set_env_shared_stack_size(size_t size);
    co_env* current_env();
    void    set_base_schedule_thread_count(size_t base_thread_count);
    void    set_max_schedule_thread_count(size_t max_thread_count);
    void    set_if_free_mem_callback(std::function<bool()> cb);
    void    set_timer_tick_duration(
           const std::chrono::steady_clock::duration& duration);
    const std::chrono::steady_clock::duration& timing_duration() const;
    ~co_manager();

    friend class co_singleton_static<co_manager>;
    friend class co_timer;
};

CO_NAMESPACE_END