#include "co_shared_mutex.h"
#include "co.h"
#include "co_error.h"

#include <cassert>
#include <mutex>

CO_NAMESPACE_BEGIN

void co_shared_mutex::lock()
{
    auto ctx = co::current_ctx();

    lock_context context {
        lock_type::unique,
        ctx
    };

    std::unique_lock<co_spinlock> lck(spinlock__);
    if (owners__.empty())
    {
        owners__.push_back(context);
        return;
    }

    ctx->set_flag(CO_CTX_FLAG_WAITING);
    wait_list__.push_back(context);

    while (owners__.empty() || owners__.front().ctx != ctx || owners__.front().type != lock_type::unique)
    {
        lck.unlock();
        this_co::yield();
        lck.lock();
    }
}

bool co_shared_mutex::try_lock()
{
    auto ctx = co::current_ctx();

    lock_context context {
        lock_type::unique,
        ctx
    };

    std::unique_lock<co_spinlock> lck(spinlock__);
    if (owners__.empty())
    {
        owners__.push_back(context);
        return true;
    }
    return false;
}

void co_shared_mutex::unlock_writer__(co_ctx* ctx)
{
    if (owners__.empty() || owners__.front().ctx != ctx || owners__.front().type != lock_type::unique)
    {
        CO_O_ERROR("ctx is not owner, this ctx is %p", ctx);
        throw co_error("ctx is not owner[", ctx, "]");
    }

    assert(owners__.size() == 1);

    owners__.clear();
}

void co_shared_mutex::unlock()
{
    auto ctx = co::current_ctx();

    unlock_writer__(ctx);

    select_competitors_to_wake_up__();
    wake_up_owners__();
}

void co_shared_mutex::wake_up_owners__()
{
    for (auto& c : owners__)
    {
        c.ctx->reset_flag(CO_CTX_FLAG_WAITING);
        c.ctx->env()->wake_up();
    }
}

void co_shared_mutex::reader_wait__(co_ctx* ctx, std::unique_lock<co_spinlock>& lck)
{
    while (true)
    {
        for (auto& c : owners__)
        {
            if (c.type != lock_type::shared)
            {
                break;
            }
            if (c.ctx == ctx)
            {
                return;
            }
        }

        lck.unlock();
        this_co::yield();
        lck.lock();
    }
}

void co_shared_mutex::lock_shared()
{
    auto ctx = co::current_ctx();

    lock_context context {
        lock_type::shared,
        ctx
    };

    std::unique_lock<co_spinlock> lck(spinlock__);
    if (owners__.empty())
    {
        owners__.push_back(context);
        return;
    }

    auto curr_type = owners__.front().type;
    if (curr_type == lock_type::shared)
    {
        owners__.push_back(context);
        return;
    }

    ctx->set_flag(CO_CTX_FLAG_WAITING);
    wait_list__.push_back(context);

    reader_wait__(ctx, lck);
}

bool co_shared_mutex::try_lock_shared()
{
    auto ctx = co::current_ctx();

    lock_context context {
        lock_type::shared,
        ctx
    };

    std::unique_lock<co_spinlock> lck(spinlock__);
    if (owners__.empty())
    {
        owners__.push_back(context);
        return true;
    }

    auto curr_type = owners__.front().type;
    if (curr_type == lock_type::shared)
    {
        owners__.push_back(context);
        return true;
    }

    return false;
}

void co_shared_mutex::unlock_reader__(co_ctx* ctx)
{
    if (owners__.empty())
    {
        CO_O_ERROR("ctx is not owner, this ctx is %p", ctx);
        throw co_error("ctx is not owner[", ctx, "]");
    }

    // 判断是否拥有锁
    bool is_owner = false;
    for (auto iter = owners__.begin(); iter != owners__.end(); ++iter)
    {
        if (iter->type != lock_type::shared)
        {
            CO_O_ERROR("ctx is not owner, this ctx is %p", ctx);
            throw co_error("ctx is not owner[", ctx, "]");
        }
        if (iter->ctx == ctx)
        {
            is_owner = true;
            owners__.erase(iter); // 拥有锁，解锁
            break;
        }
    }

    if (!is_owner)
    {
        CO_O_ERROR("ctx is not owner, this ctx is %p", ctx);
        throw co_error("ctx is not owner[", ctx, "]");
    }
}

void co_shared_mutex::select_all_reader_to_wake_up__()
{
    auto iter = std::remove_if(wait_list__.begin(), wait_list__.end(), [](const lock_context& c) {
        return c.type == lock_type::shared;
    });
    owners__.insert(owners__.begin(), iter, wait_list__.end());
    wait_list__.erase(iter, wait_list__.end());
}

void co_shared_mutex::unlock_shared()
{
    auto ctx = co::current_ctx();

    unlock_reader__(ctx);

    //  此处已经解锁了
    if (!owners__.empty()) // 如果共享不为空，将后续等待的共享者全部唤醒
    {
        select_all_reader_to_wake_up__();
    }
    else
    {
        select_competitors_to_wake_up__();
    }

    wake_up_owners__();
}

void co_shared_mutex::select_competitors_to_wake_up__()
{
    if (wait_list__.empty())
    {
        return;
    }
    auto next_type = wait_list__.front().type;
    if (next_type == lock_type::unique)
    {
        owners__.push_back(wait_list__.front());
        wait_list__.pop_front();
    }
    else
    {
        select_all_reader_to_wake_up__();
    }
}

CO_NAMESPACE_END