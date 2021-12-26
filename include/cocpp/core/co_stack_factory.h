_Pragma("once");
#include "cocpp/core/co_define.h"
#include "cocpp/mem/co_mem_pool.h"
#include "cocpp/mem/co_object_pool.h"
#include "cocpp/utils/co_noncopyable.h"
#include "cocpp/utils/co_singleton.h"
#include <cstddef>

CO_NAMESPACE_BEGIN

class co_stack;
class co_manager;

class co_stack_factory final : public co_singleton<co_stack_factory>
{
private:
    co_object_pool<co_stack> stack_pool__ { MAX_STACK_CACHE_COUNT };                                            // 堆栈对象池
    co_mem_pool              mem_pool__ { MIN_STACK_MEM_ZONE, MAX_STACK_ZONE_COUNT, MAX_MEM_POOL_CACHE_COUNT }; //   堆栈内存池
public:                                                                                                         //
    co_stack* create_stack(size_t size);                                                                        // 创建堆栈
    void      destroy_stack(co_stack* stack);                                                                   // 销毁堆栈
    void      free_stack_mem_pool();                                                                            // 释放堆栈内存池
    void      free_obj_pool();                                                                                  // 释放堆栈对象池
};

CO_NAMESPACE_END