#include "co_spinlock.h"

#include "co.h"

void co_spinlock::lock_index__()
{
    bool lock = false;
    while (!token_locked__.compare_exchange_strong(lock, true))
    {
        lock = false;
    }
}

void co_spinlock::unlock_index__()
{
    bool lock = true;
    while (!token_locked__.compare_exchange_strong(lock, false))
    {
        lock = true;
    }
}

void co_spinlock::lock()
{
    lock_index__();
    unsigned long long my_key = token__;
    ++token__;
    unsigned long long token = 0;
    req_list__.push_back(&token);
    unlock_index__();
    // my_index 与 token 相差1，只需要给my_key增加1就可以跳出循环
    while (my_key != token)
    {
        co::schedule_switch();
    }
}

void co_spinlock::unlock()
{
    lock_index__();
    auto token = req_list__.front();
    req_list__.pop_front();
    unlock_index__();
    // 唤醒下一个等待者
    ++(*token);
}