#include "co_o1_scheduler.h"
#include "co_ctx.h"
#include "co_define.h"
#include "co_entry.h"
#include <cassert>
#include <mutex>

CO_NAMESPACE_BEGIN

co_o1_scheduler::co_o1_scheduler()
    : all_ctx__(CO_MAX_PRIORITY)
{
}

void co_o1_scheduler::add_ctx(co_ctx* obj)
{
    std::lock_guard<std::mutex> lock(mu_all_ctx__);
    all_ctx__[obj->priority()].push_back(obj);
    // CO_O_DEBUG("add obj %s %p , state: %d\n", obj->config().name.c_str(), obj, (int)obj->state());
}

void co_o1_scheduler::remove_ctx(co_ctx* obj)
{
    std::lock_guard<std::mutex> lock(mu_all_ctx__);
    // CO_O_DEBUG("remove obj %s %p , state: %d", obj->config().name.c_str(), obj, (int)obj->state());
    // 此处不能断言 curr__ != ctx，因为在最后清理所有的ctx的时候，可以删除当前ctx
    all_ctx__[obj->priority()].remove(obj);
}

co_ctx* co_o1_scheduler::choose_ctx()
{
    std::lock_guard<std::mutex> lock(mu_all_ctx__);
    for (unsigned int i = 0; i < all_ctx__.size(); ++i)
    {
        if (all_ctx__[i].empty())
        {
            continue;
        }

        for (auto& obj : all_ctx__[i])
        {
            if (obj->can_schedule())
            {
                auto ret = obj;
                all_ctx__[i].remove(obj);
                all_ctx__[i].push_back(ret);
                curr_obj__ = ret;
                return ret;
            }
        }
    }
    curr_obj__ = nullptr;
    return nullptr;
}

std::list<co_ctx*> co_o1_scheduler::all_ctx() const
{
    std::lock_guard<std::mutex> lock(mu_all_ctx__);
    std::list<co_ctx*>          ret;
    for (auto& lst : all_ctx__)
    {
        ret.insert(ret.end(), lst.begin(), lst.end());
    }
    return ret;
}

size_t co_o1_scheduler::count() const
{
    std::lock_guard<std::mutex> lock(mu_all_ctx__);

    size_t ret = 0;
    for (unsigned int i = 0; i < all_ctx__.size(); ++i)
    {
        ret += all_ctx__[i].size();
    }
    return ret;
}

co_ctx* co_o1_scheduler::current_ctx() const
{
    std::lock_guard<std::mutex> lock(mu_all_ctx__);

    return curr_obj__;
}

bool co_o1_scheduler::can_schedule() const
{
    std::lock_guard<std::mutex> lock(mu_all_ctx__);
    for (unsigned int i = 0; i < all_ctx__.size(); ++i)
    {
        for (auto& obj : all_ctx__[i])
        {
            if (obj->can_schedule())
            {
                return true;
            }
        }
    }
    return false;
}

void co_o1_scheduler::change_priority(int old, co_ctx* obj)
{
    std::lock_guard<std::mutex> lock(mu_all_ctx__);
    for (auto iter = all_ctx__[old].begin(); iter != all_ctx__[old].end(); ++iter)
    {
        if (*iter == obj)
        {
            all_ctx__[old].erase(iter);
            all_ctx__[obj->priority()].push_back(obj);
            return;
        }
    }
    assert(false);
}

CO_NAMESPACE_END