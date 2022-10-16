_Pragma("once");

#include "cocpp/core/co_define.h"
#include "cocpp/utils/co_any.h"

CO_NAMESPACE_BEGIN

class co_return_value
{
    co_any value__;

public:
    co_return_value(co_any value);
    co_return_value(const co_return_value &value) = default;
    co_return_value &operator=(const co_return_value &value) = default;
    co_return_value &operator=(co_return_value &&value) = default;

    template <typename T>
    operator T();
};

template <typename T>
co_return_value::operator T()
{
    return value__.get<T>();
}

CO_NAMESPACE_END