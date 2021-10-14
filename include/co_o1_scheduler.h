#pragma once

#include "co_define.h"
#include "co_scheduler.h"

#include <list>
#include <mutex>
#include <vector>

CO_NAMESPACE_BEGIN

class co_o1_scheduler : public co_scheduler
{
private:
    std::vector<std::list<schedulable*>> all_obj__;
    mutable std::mutex                   mu_all_obj__;
    schedulable*                         curr_obj__ { nullptr };

    co_o1_scheduler();

public:
    void                    add_obj(schedulable* obj) override;
    void                    remove_obj(schedulable* obj) override;
    schedulable*            choose_obj() override;
    std::list<schedulable*> all_obj() const override;
    size_t                  count() const override;
    schedulable*            current_obj() const override;
    bool                    can_schedule() const override;
    void                    change_priority(int old, schedulable* obj) override;

    friend class co_o1_scheduler_factory;
};

CO_NAMESPACE_END