#pragma once

#include <list>
#include <mutex>

#include "co_define.h"
#include "co_nocopy.h"
#include "co_spinlock.h"

CO_NAMESPACE_BEGIN

class co_ctx* ctx;

class co_shared_mutex : private co_nocopy
{
private:
    enum class lock_type
    {
        unique,
        shared
    };

    struct lock_context
    {
        lock_type type;
        co_ctx*   ctx;
    };

    co_spinlock             spinlock__;
    std::list<lock_context> wait_list__;
    std::list<lock_context> owners__;

    void select_competitors_to_wake_up__();
    void wake_up_owners__();
    void unlock_reader__(co_ctx* ctx);
    void unlock_writer__(co_ctx* ctx);
    void reader_wait__(co_ctx* ctx, std::unique_lock<co_spinlock>& lck);
    void select_all_reader_to_wake_up__();

public:
    void lock();
    void unlock();
    bool try_lock();
    void lock_shared();
    void unlock_shared();
    bool try_lock_shared();
};

CO_NAMESPACE_END