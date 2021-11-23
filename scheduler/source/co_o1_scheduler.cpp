#include "co_o1_scheduler.h"
#include "co_define.h"
#include "co_entry.h"
#include <cassert>
#include <mutex>

CO_NAMESPACE_BEGIN

co_o1_scheduler::co_o1_scheduler()
    : all_obj__(CO_MAX_PRIORITY)
{
}

void co_o1_scheduler::add_obj(schedulable* obj)
{
    std::lock_guard<co_spinlock> lock(mu_all_obj__);
    all_obj__[obj->priority()].push_back(obj);
    // CO_O_DEBUG("add obj %s %p , state: %d\n", obj->config().name.c_str(), obj, (int)obj->state());
}

void co_o1_scheduler::remove_obj(schedulable* obj)
{
    std::lock_guard<co_spinlock> lock(mu_all_obj__);
    // CO_O_DEBUG("remove obj %s %p , state: %d", obj->config().name.c_str(), obj, (int)obj->state());
    // 此处不能断言 curr__ != ctx，因为在最后清理所有的ctx的时候，可以删除当前ctx
    all_obj__[obj->priority()].remove(obj);
}

schedulable* co_o1_scheduler::choose_obj()
{
    std::lock_guard<co_spinlock> lock(mu_all_obj__);
    for (unsigned int i = 0; i < all_obj__.size(); ++i)
    {
        if (all_obj__[i].empty())
        {
            continue;
        }

        for (auto& obj : all_obj__[i])
        {
            if (obj->can_schedule())
            {
                auto ret = obj;
                all_obj__[i].remove(obj);
                all_obj__[i].push_back(ret);
                curr_obj__ = ret;
                return ret;
            }
        }
    }
    curr_obj__ = nullptr;
    return nullptr;
}

std::list<schedulable*> co_o1_scheduler::all_obj() const
{
    std::lock_guard<co_spinlock> lock(mu_all_obj__);
    std::list<schedulable*>      ret;
    for (auto& lst : all_obj__)
    {
        ret.insert(ret.end(), lst.begin(), lst.end());
    }
    return ret;
}

size_t co_o1_scheduler::count() const
{
    std::lock_guard<co_spinlock> lock(mu_all_obj__);

    size_t ret = 0;
    for (unsigned int i = 0; i < all_obj__.size(); ++i)
    {
        ret += all_obj__[i].size();
    }
    return ret;
}

schedulable* co_o1_scheduler::current_obj() const
{
    std::lock_guard<co_spinlock> lock(mu_all_obj__);

    return curr_obj__;
}

bool co_o1_scheduler::can_schedule() const
{
    std::lock_guard<co_spinlock> lock(mu_all_obj__);
    for (unsigned int i = 0; i < all_obj__.size(); ++i)
    {
        for (auto& obj : all_obj__[i])
        {
            if (obj->can_schedule())
            {
                return true;
            }
        }
    }
    return false;
}

void co_o1_scheduler::change_priority(int old, schedulable* obj)
{
    std::lock_guard<co_spinlock> lock(mu_all_obj__);
    for (auto iter = all_obj__[old].begin(); iter != all_obj__[old].end(); ++iter)
    {
        if (*iter == obj)
        {
            all_obj__[old].erase(iter);
            all_obj__[obj->priority()].push_back(obj);
            return;
        }
    }
    assert(false);
}

CO_NAMESPACE_END