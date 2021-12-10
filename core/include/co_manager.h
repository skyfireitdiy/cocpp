_Pragma("once");

#include "co_ctx_config.h"
#include "co_ctx_factory.h"
#include "co_define.h"
#include "co_env_factory.h"
#include "co_event.h"
#include "co_noncopyable.h"
#include "co_singleton.h"
#include "co_spinlock.h"
#include "co_stack_factory.h"

#include <condition_variable>
#include <deque>
#include <functional>
#include <future>
#include <list>
#include <map>
#include <unordered_set>

CO_NAMESPACE_BEGIN

class co_env;
class co_ctx;
class co_env_factory;
class co_ctx_factory;
class co_stack_factory;
class co_scheduler_factory;

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

    bool clean_up__ { false };

    std::list<std::future<void>> background_task__;

    co_factory_set factory_set__ {
        .env_factory   = co_env_factory::instance(),
        .ctx_factory   = co_ctx_factory::instance(),
        .stack_factory = co_stack_factory::instance()
    };

    mutable co_spinlock                          mu_timing_duration__ { co_spinlock::lock_type::in_thread };
    std::chrono::high_resolution_clock::duration timing_duration__ { std::chrono::milliseconds(10) };

    size_t default_shared_stack_size__ = CO_DEFAULT_STACK_SIZE;

    std::function<bool()> need_free_mem_cb__ { [] { return false; } };

    void    clean_env_routine__();
    void    timing_routine__();
    void    redistribute_ctx__();
    void    force_schedule__();
    void    destroy_redundant_env__();
    void    free_mem__();
    void    wait_background_task__();
    void    set_clean_up__();
    void    destroy_all_factory__();
    co_env* get_best_env__();
    void    remove_env__(co_env* env);
    void    sub_env_event__(co_env* env);
    void    sub_ctx_event__(co_ctx* env);
    void    sub_manager_event__();
    void    create_background_task__();
    void    create_env_from_this_thread__();

    co_manager();

public:
    co_env* create_env(bool dont_auto_destory);
    co_ctx* create_and_schedule_ctx(const co_ctx_config& config, bool lock_destroy = true);
    void    set_env_shared_stack_size(size_t size);
    co_env* current_env();
    void    set_base_schedule_thread_count(size_t base_thread_count);
    void    set_max_schedule_thread_count(size_t max_thread_count);
    void    set_if_gc_callback(std::function<bool()> cb);
    void    set_timing_tick_duration(
           const std::chrono::high_resolution_clock::duration& duration);
    const std::chrono::high_resolution_clock::duration& timing_duration() const;

    ~co_manager();

    friend class co_singleton_static<co_manager>;
};

CO_NAMESPACE_END