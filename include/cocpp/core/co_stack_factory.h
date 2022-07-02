_Pragma("once");
#include "cocpp/core/co_define.h"
#include "cocpp/mem/co_object_pool.h"
#include "cocpp/utils/co_singleton.h"
#include <cstddef>

CO_NAMESPACE_BEGIN

class co_stack;
class co_manager;

class co_stack_factory final : public co_singleton<co_stack_factory>
{
private:
    co_object_pool<co_stack> stack_pool__ { MAX_STACK_CACHE_COUNT };

public:
    co_stack* create_stack(size_t size);
    void      destroy_stack(co_stack* stack);
    void      free_obj_pool();
};

CO_NAMESPACE_END