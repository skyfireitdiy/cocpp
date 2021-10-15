#pragma once

#include "co_define.h"

#include <any>

CO_NAMESPACE_BEGIN

// 协程返回值对象
class co_return_value
{
    // 用来存储返回值
    std::any value__;

public:
    co_return_value(const std::any& value);
    co_return_value() = default;

    // 类型转换操作符，如果类型不匹配会抛出异常
    template <typename T>
    operator T();
};

template <typename T>
co_return_value::operator T()
{
    return std::any_cast<T>(value__);
}

CO_NAMESPACE_END