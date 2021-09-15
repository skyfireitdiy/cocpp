#pragma once

#include <any>

// 协程返回值对象
class co_ret
{
    // 用来存储返回值
    std::any value__;

public:
    co_ret(const std::any& value);
    co_ret() = default;

    // 类型转换操作符，如果类型不匹配会抛出异常
    template <typename T>
    operator T()
    {
        return std::any_cast<T>(value__);
    }
};
