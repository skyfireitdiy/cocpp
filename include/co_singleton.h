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
    static T* instance();

    friend T;
};

template <typename T>
T* co_singleton<T>::instance()
{
    static T inst;
    return &inst;
}