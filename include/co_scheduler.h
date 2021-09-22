#pragma once

#include <list>

class co_ctx;

class co_scheduler
{
public:
    virtual void               add_ctx(co_ctx* ctx)    = 0;
    virtual void               remove_ctx(co_ctx* ctx) = 0;
    virtual co_ctx*            choose_ctx()            = 0;
    virtual std::list<co_ctx*> all_ctx() const         = 0;
    virtual size_t             count() const           = 0;
    virtual co_ctx*            current_ctx() const     = 0;
    virtual bool               can_schedule() const    = 0;
};