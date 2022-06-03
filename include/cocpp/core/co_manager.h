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
    RegCoEvent(timing_routine_timout);

private:
    struct
    {
        std::set<co_env*>           normal_set;
        std::recursive_mutex        normal_lock;
        std::set<co_env*>           expired_set;
        std::condition_variable_any cv_expired_env;
        unsigned int                base_env_count;
        unsigned int                max_env_count;
    } env_set__ {
        .base_env_count = std::thread::hardware_concurrency(),
        .max_env_count  = std::thread::hardware_concurrency() * 2
    };

    // Other threads can access this data.
    bool clean_up__ { false };

    mutable std::recursive_mutex        mu_timer_duration__;
    std::chrono::steady_clock::duration timer_duration__ { 10ms };

    std::function<bool()> need_free_mem_cb__ { [] { return false; } };
    std::recursive_mutex  need_free_mem_cb_lock__;

    std::set<std::shared_ptr<co_timer>> timer_queue__;
    std::recursive_mutex                mu_timer_queue__;
    std::condition_variable_any         cv_timer_queue__;

    std::list<std::future<void>> background_task__;

    size_t default_shared_stack_size__ = CO_DEFAULT_STACK_SIZE;

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
    std::string manager_info();

    friend class co_singleton_static<co_manager>;
    friend class co_timer;
};

CO_NAMESPACE_END