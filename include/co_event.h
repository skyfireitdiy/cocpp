#pragma once

#include "co_define.h"
#include "co_nocopy.h"
#include "co_spinlock.h"
#include <functional>
#include <list>
#include <mutex>
#include <utility>

CO_NAMESPACE_BEGIN

template <typename... Args>
using co_event_handler_type = typename std::list<std::function<void(Args&&... args)>>::const_iterator;

template <typename... Args>
class co_event final : public co_nocopy
{
private:
    std::list<std::function<void(Args&&... args)>> cb_list__;
    mutable co_spinlock                            spinlock__;

public:
    using handler = typename std::list<std::function<void(Args&&... args)>>::const_iterator;
    handler register_callback(std::function<void(Args&&... args)> cb);
    void    emit(Args&&... args) const;
    void    remove_callback(handler h);
};

// 模板实现

template <typename... Args>
typename co_event<Args...>::handler co_event<Args...>::register_callback(std::function<void(Args&&... args)> cb)
{
    std::lock_guard<co_spinlock> lck(spinlock__);
    return cb_list__.insert(cb_list__.end(), cb);
}

template <typename... Args>
void co_event<Args...>::remove_callback(typename co_event<Args...>::handler h)
{
    std::lock_guard<co_spinlock> lck(spinlock__);
    cb_list__.erase(h);
}

template <typename... Args>
void co_event<Args...>::emit(Args&&... args) const
{
    std::lock_guard<co_spinlock> lck(spinlock__);
    for (auto& cb : cb_list__)
    {
        cb(std::forward<Args>(args)...);
    }
}

CO_NAMESPACE_END

#define RegCoEvent(eventName, ...)       \
private:                                 \
    co_event<__VA_ARGS__> eventName##__; \
                                         \
public:                                  \
    co_event<__VA_ARGS__>& eventName()   \
    {                                    \
        return eventName##__;            \
    }
