#pragma once
#include <utility>

#include "co_define.h"
#include "co_nocopy.h"

CO_NAMESPACE_BEGIN

// 单例模式
template <typename T>
class co_singleton : private co_nocopy
{
private:
    co_singleton() = default;

public:
    // 获取实例
    static T*   instance();
    static void destroy_instance();
    friend T;
};

// 单例模式（可回收）
template <typename T>
class co_singleton_static : private co_nocopy
{
private:
    co_singleton_static() = default;

public:
    // 获取实例
    static T* instance();
    friend T;
};

template <typename T>
T* co_singleton<T>::instance()
{
    static T* inst = new T;
    return inst;
}

template <typename T>
void co_singleton<T>::destroy_instance()
{
    delete instance();
}

template <typename T>
T* co_singleton_static<T>::instance()
{
    static T inst;
    return &inst;
}

CO_NAMESPACE_END