#pragma once

#include <list>

#include "co_define.h"
#include "co_nocopy.h"

CO_NAMESPACE_BEGIN

class schedulable
{
public:
    virtual int  priority() const           = 0;
    virtual void set_priority(int priority) = 0;
    virtual bool can_schedule() const       = 0;

    virtual ~schedulable() = default;
};

class co_scheduler : public co_nocopy
{
public:
    virtual void                    add_obj(schedulable* obj)                  = 0;
    virtual void                    remove_obj(schedulable* obj)               = 0;
    virtual schedulable*            choose_obj()                               = 0;
    virtual std::list<schedulable*> all_obj() const                            = 0;
    virtual size_t                  count() const                              = 0;
    virtual schedulable*            current_obj() const                        = 0;
    virtual bool                    can_schedule() const                       = 0;
    virtual void                    change_priority(int old, schedulable* obj) = 0;

    virtual ~co_scheduler() = default;
};

CO_NAMESPACE_END