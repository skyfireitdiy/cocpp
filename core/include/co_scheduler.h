#pragma once

#include <list>

#include "co_define.h"
#include "co_nocopy.h"

CO_NAMESPACE_BEGIN

class co_ctx;

class co_schedulable_obj
{
private:
    co_ctx* ctx__;

public:
    virtual int  priority() const           = 0;
    virtual void set_priority(int priority) = 0;
    virtual bool can_schedule() const       = 0;
    virtual ~co_schedulable_obj()           = default;

    co_ctx* scheduler_ctx() const;
    void    set_scheduler_ctx(co_ctx* ctx);
};

class co_scheduler : private co_nocopy
{
public:
    virtual void                           add_obj(co_schedulable_obj* obj)                  = 0;
    virtual void                           remove_obj(co_schedulable_obj* obj)               = 0;
    virtual co_schedulable_obj*            choose_obj()                                      = 0;
    virtual std::list<co_schedulable_obj*> all_obj() const                                   = 0;
    virtual size_t                         count() const                                     = 0;
    virtual co_schedulable_obj*            current_obj() const                               = 0;
    virtual bool                           can_schedule() const                              = 0;
    virtual void                           change_priority(int old, co_schedulable_obj* obj) = 0;

    virtual ~co_scheduler() = default;
};

CO_NAMESPACE_END