#pragma once

#include "co_define.h"
#include "co_scheduler.h"
#include "co_spinlock.h"

#include <list>
#include <mutex>
#include <vector>

CO_NAMESPACE_BEGIN

class co_o1_scheduler : public co_scheduler
{
private:
    std::vector<std::list<co_schedulable_obj*>> all_obj__;
    mutable std::mutex                          mu_all_obj__;
    co_schedulable_obj*                         curr_obj__ { nullptr };

    co_o1_scheduler();

public:
    void                           add_obj(co_schedulable_obj* obj) override;
    void                           remove_obj(co_schedulable_obj* obj) override;
    co_schedulable_obj*            choose_obj() override;
    std::list<co_schedulable_obj*> all_obj() const override;
    size_t                         count() const override;
    co_schedulable_obj*            current_obj() const override;
    bool                           can_schedule() const override;
    void                           change_priority(int old, co_schedulable_obj* obj) override;

    friend class co_o1_scheduler_factory;
};

CO_NAMESPACE_END