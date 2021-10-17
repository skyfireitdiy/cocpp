#pragma once

#include "co_define.h"
#include "co_type.h"
#include <cassert>
#include <list>
#include <mutex>

CO_NAMESPACE_BEGIN

template <typename ObjectType>
class co_object_pool final
{
private:
    std::list<void*> pool__;
    std::mutex       mu__;

public:
    template <typename... ConstructParam>
    ObjectType* create_obj(ConstructParam&&... params);
    void        destroy_obj(ObjectType* obj);
    void        clear_free_object();
    ~co_object_pool();
};

template <typename ObjectType>
template <typename... ConstructParam>
ObjectType* co_object_pool<ObjectType>::create_obj(ConstructParam&&... params)
{
    std::lock_guard<std::mutex> lck(mu__);
    void*                       mem = nullptr;
    if (pool__.empty())
    {
        mem = malloc(sizeof(ObjectType));
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
    std::lock_guard<std::mutex> lck(mu__);
    obj->~ObjectType();
    pool__.push_back(obj);
}

template <typename ObjectType>
void co_object_pool<ObjectType>::clear_free_object()
{
    std::lock_guard<std::mutex> lck(mu__);
    for (auto& obj : pool__)
    {
        free(obj);
    }
}

template <typename ObjectType>
co_object_pool<ObjectType>::~co_object_pool()
{
    clear_free_object();
}

CO_NAMESPACE_END