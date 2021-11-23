#pragma once

#include <list>

#include "co_define.h"
#include "co_nocopy.h"

CO_NAMESPACE_BEGIN

class schedulable_obj
{
public:
    virtual int  priority() const           = 0;
    virtual void set_priority(int priority) = 0;
    virtual bool can_schedule() const       = 0;

    virtual ~schedulable_obj() = default;
};

class co_scheduler : private co_nocopy
{
public:
    virtual void                        add_obj(schedulable_obj* obj)                  = 0;
    virtual void                        remove_obj(schedulable_obj* obj)               = 0;
    virtual schedulable_obj*            choose_obj()                                   = 0;
    virtual std::list<schedulable_obj*> all_obj() const                                = 0;
    virtual size_t                      count() const                                  = 0;
    virtual schedulable_obj*            current_obj() const                            = 0;
    virtual bool                        can_schedule() const                           = 0;
    virtual void                        change_priority(int old, schedulable_obj* obj) = 0;

    virtual ~co_scheduler() = default;
};

CO_NAMESPACE_END