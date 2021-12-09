_Pragma("once");

#include "co_define.h"
#include "co_noncopyable.h"
#include "co_type.h"
#include <cassert>
#include <cstdlib>
#include <deque>
#include <mutex>

CO_NAMESPACE_BEGIN

template <typename ObjectType>
class co_object_pool final : private co_noncopyable
{
private:
    std::deque<void*> pool__;
    co_spinlock       mu__ { false };
    size_t            max_cap__;

public:
    co_object_pool(size_t max_cap);
    template <typename... ConstructParam>
    ObjectType* create_obj(ConstructParam&&... params);
    void        destroy_obj(ObjectType* obj);
    void        clear_free_object();
    ~co_object_pool();
};

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
    pool__.push_front(obj); // 尽快用到这块内存
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