_Pragma("once");
#include "cocpp/core/co_define.h"
#include "cocpp/core/co_type.h"
#include "cocpp/mem/co_object_pool.h"
#include "cocpp/utils/co_noncopyable.h"

CO_NAMESPACE_BEGIN

class co_stack final : private co_noncopyable
{
    co_byte* stack__;                          // 堆栈指针
    size_t   size__;                           // 堆栈大小
    co_stack(co_byte* ptr, size_t stack_size); // 构造函数
public:
    size_t   stack_size() const; // 堆栈大小
    co_byte* stack() const;      // 堆栈指针
    co_byte* stack_top() const;  // 堆栈顶部指针

    friend class co_object_pool<co_stack>;
};

CO_NAMESPACE_END