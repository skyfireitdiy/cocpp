#include "cocpp/core/co_stack_factory.h"
_Pragma("once");
#include "cocpp/core/co_define.h"
#include "cocpp/core/co_type.h"
#include "cocpp/utils/co_noncopyable.h"

CO_NAMESPACE_BEGIN

class co_stack final : private co_noncopyable
{
    co_byte* stack__;
    size_t   size__;
    co_stack(co_byte* ptr, size_t stack_size);

public:
    size_t      stack_size() const;
    co_byte*    stack() const;
    co_byte*    stack_top() const;
    std::string stack_info() const;

    friend class co_stack_factory;
};

CO_NAMESPACE_END