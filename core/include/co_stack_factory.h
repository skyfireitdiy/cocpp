_Pragma("once");
#include "co_define.h"
#include "co_mem_pool.h"
#include "co_noncopyable.h"
#include "co_object_pool.h"
#include "co_singleton.h"
#include <cstddef>

CO_NAMESPACE_BEGIN

class co_stack;
class co_manager;

class co_stack_factory final : public co_singleton<co_stack_factory>
{
private:
    co_object_pool<co_stack> stack_pool__ { MAX_STACK_CACHE_COUNT };
    co_mem_pool              mem_pool__ { MIN_STACK_MEM_ZONE, MAX_STACK_ZONE_COUNT, MAX_MEM_POOL_CACHE_COUNT };

public:
    co_stack* create_stack(size_t size);
    void      destroy_stack(co_stack* stack);
    void      free_stack_mem_pool();
    void      free_obj_pool();
};

CO_NAMESPACE_END