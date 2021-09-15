#pragma once

// 单例模式
template <typename T>
class co_singleton
{
private:
    co_singleton() {}

public:
    // 获取实例
    static T* instance()
    {
        static T* inst = new T();
        return inst;
    }
    friend T;
};