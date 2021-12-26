_Pragma("once");

#include "cocpp/core/co_define.h"
#include "cocpp/core/co_type.h"
#include "cocpp/utils/co_noncopyable.h"
#include <cassert>
#include <cstdlib>
#include <deque>

CO_NAMESPACE_BEGIN

template <typename ObjectType>
class co_object_pool final : private co_noncopyable
{
private:
    std::deque<void*> pool__;                                     // 内存池
    co_spinlock       mu__ { co_spinlock::lock_type::in_thread }; // 互斥锁
    size_t            max_cap__;                                  // 最大容量
public:                                                           //
    co_object_pool(size_t max_cap);                               // 构造函数
    template <typename... ConstructParam>                         //
    ObjectType* create_obj(ConstructParam&&... params);           // 创建对象
    void        destroy_obj(ObjectType* obj);                     // 销毁对象
    void        clear_free_object();                              // 清空空闲对象
    ~co_object_pool();                                            // 析构函数
};

// 模板实现

template <typename ObjectType>
co_object_pool<ObjectType>::co_object_pool(size_t max_cap)
    : max_cap__(max_cap)
{
}

template <typename ObjectType>
template <typename... ConstructParam>
ObjectType* co_object_pool<ObjectType>::create_obj(ConstructParam&&... params)
{
    std::lock_guard<co_spinlock> lck(mu__);
    void*                        mem = nullptr;
    if (pool__.empty())
    {
        mem = std::aligned_alloc(alignof(ObjectType), sizeof(ObjectType));
    }
    else
    {
        mem = pool__.front();
        pool__.pop_front();
    }
    assert(mem != nullptr);
    return new (mem) ObjectType(std::forward<ConstructParam>(params)...);
}

template <typename ObjectType>
void co_object_pool<ObjectType>::destroy_obj(ObjectType* obj)
{
    std::lock_guard<co_spinlock> lck(mu__);
    obj->~ObjectType();
    if (pool__.size() > max_cap__)
    {
        free(obj);
    }
    else
    {
        pool__.push_front(obj); // 尽快用到这块内存
    }
}

template <typename ObjectType>
void co_object_pool<ObjectType>::clear_free_object()
{
    std::lock_guard<co_spinlock> lck(mu__);
    for (auto& obj : pool__)
    {
        free(obj);
    }
    pool__.clear();
}

template <typename ObjectType>
co_object_pool<ObjectType>::~co_object_pool()
{
    clear_free_object();
}

CO_NAMESPACE_END