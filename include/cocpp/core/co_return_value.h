_Pragma("once");

#include "cocpp/core/co_define.h"
#include "cocpp/utils/co_any.h"

CO_NAMESPACE_BEGIN

// 协程返回值对象
class co_return_value
{
    co_any value__; // 用来存储返回值
public:
    co_return_value(co_any value);                                      // 构造函数
    co_return_value(const co_return_value& value) = default;            // 构造函数
    co_return_value& operator=(const co_return_value& value) = default; // 赋值操作符
    co_return_value& operator=(co_return_value&& value) = default;      // 赋值操作符

    template <typename T>
    operator T(); // 类型转换操作符，如果类型不匹配会抛出异常
};

template <typename T>
co_return_value::operator T()
{
    return value__.get<T>();
}

CO_NAMESPACE_END