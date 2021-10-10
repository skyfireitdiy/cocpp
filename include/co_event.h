#pragma once

#include "co_nocopy.h"
#include "co_spinlock.h"
#include <functional>
#include <mutex>
#include <utility>

template <typename... Args>
class co_event final : public co_nocopy
{
private:
    std::list<std::function<void(Args&&... args)>> cb_list__;
    co_spinlock                                    spinlock__;

public:
    void register_callback(std::function<void(Args&&... args)> cb);
    void emit(Args&&... args);
};

// 模板实现

template <typename... Args>
void co_event<Args...>::register_callback(std::function<void(Args&&... args)> cb)
{
    std::lock_guard<co_spinlock> lck(spinlock__);
    cb_list__.push_back(cb);
}

template <typename... Args>
void co_event<Args...>::emit(Args&&... args)
{
    std::lock_guard<co_spinlock> lck(spinlock__);
    for (auto& cb : cb_list__)
    {
        cb(std::forward<Args>(args)...);
    }
}