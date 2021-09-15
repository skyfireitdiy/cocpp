#include "co_default_env.h"
#include "co_ctx_factory.h"
#include "co_define.h"
#include "co_env_factory.h"
#include "co_manager.h"
#include "co_type.h"

#include <cassert>
#include <future>
#include <iterator>

#ifdef _MSC_VER
#ifdef _WIN64
extern "C" void __switch_to_msvc_x64(co_byte**, co_byte**);
#else
#error only support x86_64 in msvc
#endif
#endif

co_default_env::co_default_env(co_ctx* idle_ctx, co_stack* shared_stack, bool create_new_thread)
    : idle_ctx__(idle_ctx)
    , shared_stack__(shared_stack)
    , state__(co_env_state::created)
{
    all_ctx__.push_back(idle_ctx__);
    if (create_new_thread)
    {
        // 此处设置状态是防止 add_ctx 前调度线程还未准备好，状态断言失败
        state__ = co_env_state::idle;
        start_schedule();
    }
    else
    {
        current_env__ = this;
    }
}

co_stack* co_default_env::shared_stack() const
{
    return shared_stack__;
}

void co_default_env::add_ctx(co_ctx* ctx)
{
    assert(state__ != co_env_state::created);
    ctx->set_env(this);
    std::lock_guard<std::mutex> lck(mu_all_ctx__);
    all_ctx__.push_back(ctx);
    state__ = co_env_state::busy;
}

std::optional<co_ret> co_default_env::wait_ctx(co_ctx*& ctx, const std::chrono::milliseconds& timeout)
{
    std::optional<co_ret> ret;
    auto                  now = std::chrono::system_clock::now();
    while (ctx->state() != co_state::finished)
    {
        if (std::chrono::system_clock::now() - now > timeout)
        {
            return ret;
        }
        schedule_switch();
    }
    ret = ctx->ret_ref();
    ctx->env()->remove_ctx(ctx);
    ctx = nullptr;
    return ret;
}

co_ret co_default_env::wait_ctx(co_ctx*& ctx)
{
    while (ctx->state() != co_state::finished)
    {
        schedule_switch();
    }
    co_ret ret = ctx->ret_ref();
    ctx->env()->remove_ctx(ctx);
    ctx = nullptr;
    return ret;
}

int co_default_env::workload() const
{
    std::lock_guard<std::mutex> lck(mu_all_ctx__);
    return all_ctx__.size();
}

bool co_default_env::has_scheduler_thread() const
{
    return state__ != co_env_state::created;
}

co_env_state co_default_env::state() const
{
    return state__;
}

void co_default_env::set_state(co_env_state state)
{
    state__ = state;
}

void co_default_env::remove_detached_ctx__()
{
    std::lock_guard<std::mutex> lck(mu_all_ctx__);

    auto pos = std::remove_if(
        all_ctx__.begin(),
        all_ctx__.end(),
        [](auto& ctx) {
            return ctx->state() == co_state::finished && ctx->detach();
        });

    for (auto p = pos; p != all_ctx__.end(); ++p)
    {
        manager__->ctx_factory()->destroy_ctx(*p);
    }

    all_ctx__.erase(pos, all_ctx__.end());
}

void co_default_env::schedule_switch()
{
    remove_detached_ctx__();

    auto curr = current_ctx();
    assert(curr != nullptr);
    co_ctx* next = nullptr;

    {
        std::lock_guard<std::mutex> lck(mu_all_ctx__);
        do
        {
            current_index__ = (current_index__ + 1) % all_ctx__.size();
            next            = all_ctx__[current_index__];
        } while (next->state() == co_state::finished);
    }

    if (curr->state() != co_state::finished)
    {
        curr->set_state(co_state::suspended);
    }

    next->set_state(co_state::running);
    if (curr == next)
    {
        return;
    }

    if (next != idle_ctx__)
    {
        state__ = co_env_state::busy;
    }

    switch_to__(curr->regs(), next->regs());
}

void co_default_env::remove_ctx(co_ctx* ctx)
{
    ctx->set_env(nullptr);
    std::lock_guard<std::mutex> lck(mu_all_ctx__);
    all_ctx__.erase(std::remove(all_ctx__.begin(), all_ctx__.end(), ctx));
    manager__->ctx_factory()->destroy_ctx(ctx);

    update_state__();
}

void co_default_env::switch_to__(co_byte** curr, co_byte** next)
{
#ifdef _MSC_VER
#ifdef _WIN64
    ::__switch_to_msvc_x64(curr, next);
#else
#error only support x86_64 in msvc
#endif
#endif
}

co_ctx* co_default_env::current_ctx() const
{
    assert(!all_ctx__.empty());
    return all_ctx__[current_index__];
}

co_ctx* co_default_env::idle_ctx() const
{
    return idle_ctx__;
}

void co_default_env::stop_schedule()
{
    state__ = co_env_state::destorying;
}

void co_default_env::start_schedule()
{
    worker__ = std::async([this]() {
        current_env__ = this;
        start_schedule_routine__();
    });
}

void co_default_env::start_schedule_routine__()
{
    state__ = co_env_state::idle;
    while (state__ != co_env_state::destorying)
    {
        schedule_switch();
    }

    remove_all_ctx__();
    remove_current_env__();
}

void co_default_env::schedule_in_this_thread()
{
    start_schedule_routine__();
}

void co_default_env::set_manager(co_manager* manager)
{
    manager__ = manager;
}

co_manager* co_default_env::manager() const
{
    return manager__;
}

void co_default_env::remove_all_ctx__()
{
    std::lock_guard<std::mutex> lck(mu_all_ctx__);
    // 不销毁idle_ctx，idle_ctx应该与env一起销毁
    for (int i = 1; i < all_ctx__.size(); ++i)
    {
        all_ctx__[i]->set_env(nullptr);
        manager__->ctx_factory()->destroy_ctx(all_ctx__[i]);
    }
    // 删除除idle以外的ctx
    all_ctx__.resize(1);
    // env要销毁了，是否更新状态不重要，为了保证一致性，此处更新一下
    update_state__();
}

void co_default_env::remove_current_env__()
{
    if (current_env__ != nullptr)
    {
        manager__->remove_env(current_env__);
        current_env__ = nullptr;
    }
}

void co_default_env::update_state__()
{
    // 如果当前是待销毁状态，不更新
    if (state__ == co_env_state::destorying)
    {
        return;
    }
    // 只有一个ctx的时候，是idle_ctx，所以设置为空闲状态
    if (all_ctx__.size() <= 1)
    {
        state__ = co_env_state::idle;
    }
}