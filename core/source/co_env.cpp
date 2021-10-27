#include "co_env.h"
#include "co_ctx.h"
#include "co_ctx_config.h"
#include "co_ctx_ctl.h"
#include "co_ctx_factory.h"
#include "co_defer.h"
#include "co_define.h"
#include "co_env_factory.h"
#include "co_scheduler.h"
#include "co_stack.h"
#include "co_type.h"

#include <cassert>
#include <cstring>
#include <future>
#include <iterator>
#include <mutex>

CO_NAMESPACE_BEGIN

thread_local co_env* current_env__ = nullptr;

co_env::co_env(co_scheduler* scheduler, co_stack* shared_stack, co_ctx* idle_ctx, bool create_new_thread)
    : scheduler__(scheduler)
    , shared_stack__(shared_stack)
    , idle_ctx__(idle_ctx)
    , state__(co_env_state::created)
{
    idle_ctx__->set_env(this);
    if (create_new_thread)
    {
        // 此处设置状态是防止 add_ctx 前调度线程还未准备好，状态断言失败
        set_state(co_env_state::idle);
        start_schedule();
    }
    else
    {
        set_flag(CO_ENV_FLAG_NO_SCHE_THREAD);
        set_flag(CO_ENV_FLAG_COVERTED);
        current_env__ = this;
    }
}

void co_env::add_ctx(co_ctx* ctx)
{
    assert(ctx != nullptr);
    assert(state__ != co_env_state::created && state__ != co_env_state::destorying);

    std::lock_guard<std::recursive_mutex> lock(mu_wake_up_idle__);
    ctx->set_env(this);

    init_ctx(shared_stack__, ctx); // 初始化ctx
    ctx_inited().pub(ctx);

    scheduler__->add_obj(ctx); // 添加到调度器
    set_state(co_env_state::busy);

    // CO_O_DEBUG("add ctx wake up env: %p", this);
    wake_up();
    ctx_added().pub(ctx);
}

std::optional<co_return_value> co_env::wait_ctx(co_ctx* ctx, const std::chrono::nanoseconds& timeout)
{
    // 反转优先级，防止高优先级ctx永久等待低优先级ctx
    auto old_priority = current_ctx()->priority();
    current_ctx()->set_priority(ctx->priority());
    CoDefer([this, old_priority] {
        current_ctx()->set_priority(old_priority);
    });

    std::optional<co_return_value> ret;

    auto now = std::chrono::high_resolution_clock::now();
    while (ctx->state() != co_state::finished)
    {
        if (std::chrono::high_resolution_clock::now() - now > timeout)
        {
            wait_ctx_timeout().pub(ctx);
            return ret;
        }
        schedule_switch();
    }
    wait_ctx_finished().pub(ctx);
    return ctx->ret_ref();
}

co_return_value co_env::wait_ctx(co_ctx* ctx)
{
    auto old_priority = current_ctx()->priority();
    current_ctx()->set_priority(ctx->priority());

    CoDefer([this, old_priority] {
        current_ctx()->set_priority(old_priority);
    });

    while (ctx->state() != co_state::finished)
    {
        schedule_switch();
        // CO_O_DEBUG("ctx %s %p state: %d", ctx->config().name.c_str(), ctx, ctx->state());
    }
    wait_ctx_finished().pub(ctx);
    return ctx->ret_ref();
}

int co_env::workload() const
{
    return scheduler__->count();
}

co_env_state co_env::state() const
{
    std::shared_lock<std::shared_mutex> lock(mu_state__);
    return state__;
}

void co_env::set_state(co_env_state state)
{
    std::unique_lock<std::shared_mutex> lock(mu_state__);
    if (state__ != co_env_state::destorying) // destorying 状态不允许迁移到其他状态
    {
        co_env_state old_state = state__;
        state__                = state;
        state_changed().pub(old_state, state__);
    }
}

void co_env::remove_detached_ctx__()
{
    auto curr    = scheduler__->current_obj();
    auto all_ctx = scheduler__->all_obj();
    for (auto o : all_ctx)
    {
        auto ctx = dynamic_cast<co_ctx*>(o);
        // 注意：此处不能删除当前的ctx，如果删除了，switch_to的当前上下文就没地方保存了
        if (ctx->state() == co_state::finished && ctx->can_destroy() && ctx != curr)
        {
            scheduler__->remove_obj(o);
            ctx_factory__->destroy_ctx(ctx);
        }
    }
}

co_ctx* co_env::next_ctx__()
{
    // 如果要销毁、并且可以销毁，切换到idle销毁
    if (state() == co_env_state::destorying)
    {
        return idle_ctx__;
    }
    else
    {
        auto next = scheduler__->choose_obj();
        return dynamic_cast<co_ctx*>(next == nullptr ? idle_ctx__ : next);
    }
}

void co_env::update_ctx_state__(co_ctx* curr, co_ctx* next)
{
    // 如果当前运行的ctx已经完成，状态不变
    if (curr->state() != co_state::finished)
    {
        curr->set_state(co_state::suspended);
    }
    // CO_O_DEBUG("from %p to %p", curr, next);
    next->set_state(co_state::running);
}

void co_env::schedule_switch()
{
    co_ctx* curr = nullptr;
    co_ctx* next = nullptr;
    {
        // 切换前加锁
        std::lock_guard<std::mutex> lock(mu_schedule__);

        set_flag(CO_ENV_FLAG_SCHEDULED);

        remove_detached_ctx__();

        curr = current_ctx();
        next = next_ctx__();

        assert(curr != nullptr);
        assert(next != nullptr);

        update_ctx_state__(curr, next);
        if (curr == next)
        {
            return;
        }
        if (next != idle_ctx__)
        {
            set_state(co_env_state::busy);
        }

        if (curr->test_flag(CO_CTX_FLAG_SHARED_STACK) || next->test_flag(CO_CTX_FLAG_SHARED_STACK))
        {
            shared_stack_switch_context__.from        = curr;
            shared_stack_switch_context__.to          = next;
            shared_stack_switch_context__.need_switch = true;

            // CO_O_DEBUG("prepare from:%p to:%p", shared_stack_switch_context__.from, shared_stack_switch_context__.to);

            // 设置正在切换状态，防止被转移到其他env上，因为to已经是running状态了，所以不用担心他
            if (curr->test_flag(CO_CTX_FLAG_SHARED_STACK))
            {
                // CO_O_DEBUG("set ctx %p CO_CTX_FLAG_SWITCHING flag", curr);
                curr->set_flag(CO_CTX_FLAG_SWITCHING);
            }

            if (curr == idle_ctx__)
            {
                return;
            }

            next = idle_ctx__;
            // CO_O_DEBUG("from %p to idle %p", curr, idle_ctx__);
        }
    }
    switch_to(curr->regs(), next->regs());
    switched_to().pub(curr);
}

void co_env::remove_ctx(co_ctx* ctx)
{
    scheduler__->remove_obj(ctx);
    ctx_factory__->destroy_ctx(ctx);
    ctx_removed().pub(ctx);
}

co_ctx* co_env::current_ctx() const
{
    auto ret = scheduler__->current_obj();
    if (ret == nullptr)
    {
        return idle_ctx__;
    }
    return dynamic_cast<co_ctx*>(ret);
}

void co_env::stop_schedule()
{
    // CO_O_DEBUG("set env to destorying, %p", this);
    // created 状态说明没有调度线程，不能转为destroying状态

    std::lock_guard<std::recursive_mutex> lock(mu_wake_up_idle__);
    if (!test_flag(CO_ENV_FLAG_NO_SCHE_THREAD))
    {
        set_state(co_env_state::destorying);
    }

    // CO_O_DEBUG("%p : stop schedule wake up idle co", this);
    wake_up(); // 唤醒idle的ctx
    schedule_stopped().pub();
}

void co_env::start_schedule()
{
    worker__ = std::async([this]() {
        current_env__ = this;
        start_schedule_routine__();
    });
}

void co_env::switch_shared_stack_ctx__()
{
    shared_stack_switch_context__.need_switch = false;

    // CO_O_DEBUG("switch from:%p to:%p", shared_stack_switch_context__.from, shared_stack_switch_context__.to);

    if (shared_stack_switch_context__.from->test_flag(CO_CTX_FLAG_SHARED_STACK))
    {
        // CO_O_DEBUG("save ctx %p stack", shared_stack_switch_context__.from);
        save_shared_stack__(shared_stack_switch_context__.from);
        // 保存栈完成后退出切换状态
        shared_stack_switch_context__.from->reset_flag(CO_CTX_FLAG_SWITCHING);
    }
    if (shared_stack_switch_context__.to->test_flag(CO_CTX_FLAG_SHARED_STACK))
    {
        // CO_O_DEBUG("restore ctx %p stack", shared_stack_switch_context__.from);
        restore_shared_stack__(shared_stack_switch_context__.to);
    }

    // CO_O_DEBUG("from idle %p to %p", idle_ctx__, shared_stack_switch_context__.to);
    // 切换到to
    switch_to(idle_ctx__->regs(), shared_stack_switch_context__.to->regs());
    switched_to().pub(idle_ctx__);
}

void co_env::start_schedule_routine__()
{
    schedule_started().pub();
    reset_flag(CO_ENV_FLAG_NO_SCHE_THREAD);
    set_state(co_env_state::idle);
    while (state() != co_env_state::destorying)
    {
        // 检测是否需要切换共享栈
        if (shared_stack_switch_context__.need_switch)
        {
            // CO_O_DEBUG("need switch shared stack");
            switch_shared_stack_ctx__();
        }
        else
        {
            // CO_O_DEBUG("dont need switch shared stack");
            schedule_switch();
        }

        // 切换回来检测是否需要执行共享栈切换
        if (shared_stack_switch_context__.need_switch)
        {
            continue;
        }

        set_state(co_env_state::idle); //  切换到idle协程，说明空闲了
        remove_detached_ctx__();       // 切换回来之后，将完成的ctx删除

        // 切回idle之后，睡眠等待
        std::unique_lock<std::recursive_mutex> lock(mu_wake_up_idle__);
        // CO_O_DEBUG("idle co start wait add ctx or destroying ... %p", this);
        if (state() == co_env_state::destorying)
        {
            break;
        }
        if (!scheduler__->can_schedule())
        {
            idle_waited().pub();
            cond_wake_schedule__.wait(lock);
            idle_waked().pub();
        }
    }

    // CO_O_DEBUG("stop schedule, prepare to cleanup");

    remove_all_ctx__();
    env_task_finished().pub();
    current_env__ = nullptr;
}

void co_env::schedule_in_this_thread()
{
    this_thread_converted_to_schedule_thread().pub(std::this_thread::get_id());
    start_schedule_routine__();
}

void co_env::remove_all_ctx__()
{
    auto all_obj = scheduler__->all_obj();
    for (auto& obj : all_obj)
    {
        remove_ctx(dynamic_cast<co_ctx*>(obj));
    }
    all_ctx_removed().pub();
}

co_scheduler* co_env::scheduler() const
{
    return scheduler__;
}

bool co_env::scheduled() const
{
    return test_flag(CO_ENV_FLAG_SCHEDULED);
}

void co_env::reset_scheduled_flag()
{
    reset_flag(CO_ENV_FLAG_SCHEDULED);
    scheduled_flag_reset().pub();
}

void co_env::lock_schedule__()
{
    mu_schedule__.lock();
    schedule_locked().pub();
}

void co_env::unlock_schedule__()
{
    mu_schedule__.unlock();
    schedule_unlocked().pub();
}

std::list<co_ctx*> co_env::moveable_ctx_list__()
{
    auto               all_obj = scheduler__->all_obj();
    std::list<co_ctx*> ret;
    for (auto& o : all_obj)
    {
        auto ctx = dynamic_cast<co_ctx*>(o);
        // 绑定env的协程和当前协程不能移动
        if (!ctx->can_move())
        {
            continue;
        }
        ret.push_back(ctx);
    }
    return ret;
}

void co_env::take_ctx__(co_ctx* ctx)
{
    scheduler__->remove_obj(ctx);
    ctx_taked().pub(ctx);
}

bool co_env::can_auto_destroy() const
{
    // 如果是用户自己转换的env，不能被选中销毁
    return !test_flag(CO_ENV_FLAG_COVERTED) && !test_flag(CO_ENV_FLAG_DONT_AUTO_DESTORY);
}

void co_env::wake_up()
{
    std::lock_guard<std::recursive_mutex> lock(mu_wake_up_idle__);
    // CO_O_DEBUG("wake up env: %p", this);
    cond_wake_schedule__.notify_one();
    wakeup_notified().pub();
}

size_t co_env::get_valid_stack_size(co_ctx* ctx)
{
    return ctx->stack()->stack_top() - get_rsp(ctx);
}

void co_env::save_shared_stack__(co_ctx* ctx)
{
    auto stack_size = get_valid_stack_size(ctx);
    // CO_O_DEBUG("ctx %p valid stack size is %lu", ctx, stack_size);
    auto tmp_stack = stack_factory__->create_stack(stack_size);
    memcpy(tmp_stack->stack(), get_rsp(ctx), stack_size);
    ctx->set_stack(tmp_stack);
    shared_stack_saved().pub(ctx);
}

void co_env::restore_shared_stack__(co_ctx* ctx)
{
    // 第一次调度的时候stack为nullptr
    if (ctx->stack() != nullptr)
    {
        memcpy(shared_stack__->stack_top() - ctx->stack()->stack_size(), ctx->stack()->stack(), ctx->stack()->stack_size());
        // 销毁原stack
        stack_factory__->destroy_stack(ctx->stack());
    }
    else
    {
        // CO_O_DEBUG("ctx %p  stack is nullptr", ctx);
    }
    // 设置为共享栈
    ctx->set_stack(shared_stack__);
    shared_stack_restored().pub(ctx);
}

std::list<co_ctx*> co_env::take_moveable_ctx()
{
    lock_schedule__();
    CoDefer([this] {
        unlock_schedule__();
    });
    auto ret = moveable_ctx_list__();
    for (auto& ctx : ret)
    {
        take_ctx__(ctx);
    }
    moveable_ctx_taken().pub(ret);
    return ret;
}

CO_NAMESPACE_END