#pragma once
#include <utility>

#include "co_nocopy.h"

// 单例模式
template <typename T>
class co_singleton : public co_nocopy
{
private:
    co_singleton() = default;

public:
    // 获取实例
    template <typename... Args>
    static T* instance(Args&&... args)
    {
        static T inst(std::forward<Args>(args)...);
        return &inst;
    }
    friend T;
};