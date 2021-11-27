_Pragma("once");

#include "co_define.h"
#include "co_noncopyable.h"
#include "co_type.h"
#include <functional>
#include <list>
#include <map>
#include <mutex>
#include <utility>

CO_NAMESPACE_BEGIN

template <typename... Args>
class co_event final : private co_noncopyable
{
private:
    std::map<int, std::function<void(Args... args)>> cb_list__;
    mutable std::mutex                               mu_cb_list__;
    co_event_handler                                 current_handler__ { 0 };

public:
    co_event_handler sub(std::function<void(Args... args)> cb);
    void             pub(Args... args) const;
    void             unsub(co_event_handler h);
};

// 模板实现

template <typename... Args>
co_event_handler co_event<Args...>::sub(std::function<void(Args... args)> cb)
{
    std::lock_guard<std::mutex> lck(mu_cb_list__);
    cb_list__[current_handler__] = cb;
    return ++current_handler__;
}

template <typename... Args>
void co_event<Args...>::unsub(co_event_handler h)
{
    std::lock_guard<std::mutex> lck(mu_cb_list__);
    cb_list__.erase(h);
}

template <typename... Args>
void co_event<Args...>::pub(Args... args) const
{
    std::lock_guard<std::mutex> lck(mu_cb_list__);
    for (auto& [_, cb] : cb_list__)
    {
        cb(args...);
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
