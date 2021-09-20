#pragma once
#include <utility>

// 单例模式
template <typename T>
class co_singleton
{
private:
    co_singleton() {}

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