#pragma once

#include "co_ctx_config.h"
#include "co_ctx_factory.h"
#include "co_define.h"
#include "co_env_factory.h"
#include "co_nocopy.h"
#include "co_singleton.h"
#include "co_stack_factory.h"

#include <condition_variable>
#include <future>
#include <list>
#include <map>
#include <mutex>

CO_NAMESPACE_BEGIN

class co_env;
class co_ctx;
class co_env_factory;
class co_ctx_factory;
class co_stack_factory;
class co_scheduler_factory;

class co_manager final : public co_singleton_static<co_manager>
{
private:
    std::list<co_env*>   env_list__;
    std::recursive_mutex mu_env_list__;

    std::atomic<bool> clean_up__ = { false };

    std::list<std::future<void>> background_task__;
    std::list<co_env*>           expired_env__;
    mutable std::recursive_mutex mu_clean_up__;
    std::condition_variable_any  cond_expired_env__;

    std::atomic<unsigned int> exist_env_count__ { 0 };
    std::atomic<unsigned int> base_thread_count__ { std::thread::hardware_concurrency() };
    std::atomic<unsigned int> max_thread_count__ { std::thread::hardware_concurrency() * 2 };

    co_scheduler_factory* const scheduler_factory__ { nullptr };
    co_env_factory* const       env_factory__ { co_env_factory::instance() };

    mutable std::mutex                           mu_timing_duration__;
    std::chrono::high_resolution_clock::duration timing_duration__ { std::chrono::milliseconds(10) };

    size_t default_shared_stack_size__ = CO_DEFAULT_STACK_SIZE;

    co_env* create_env__();
    bool    can_schedule_ctx__(co_env* env) const;

    void clean_env_routine__();
    void timing_routine__();
    bool is_blocked__(co_env* env) const;

    void redistribute_ctx__();
    void destroy_redundant_env__();
    void wait_background_task__();
    void set_clean_up__();
    void destroy_all_factory__();

    co_manager();

public:
    co_env*               get_best_env();
    void                  set_env_shared_stack_size(size_t size);
    co_scheduler_factory* scheduler_factory();
    void                  remove_env(co_env* env);
    void                  create_env_from_this_thread();
    co_env*               current_env();
    bool                  clean_up() const;
    void                  set_base_schedule_thread_count(size_t base_thread_count);
    void                  set_max_schedule_thread_count(size_t max_thread_count);
    void                  set_timing_duration(
                         const std::chrono::high_resolution_clock::duration& duration);
    const std::chrono::high_resolution_clock::duration& timing_duration() const;
    ~co_manager();

    friend class co_singleton_static<co_manager>;
};

CO_NAMESPACE_END