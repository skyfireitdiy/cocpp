_Pragma("once");
#include "cocpp/core/co_define.h"
#include "cocpp/utils/co_singleton.h"
#include <cstddef>

CO_NAMESPACE_BEGIN

class co_stack;
class co_manager;

class co_stack_factory
{

public:
    static co_stack *create_stack(size_t size);
    static void destroy_stack(co_stack *stack);
};

CO_NAMESPACE_END