#pragma once

class co_scheduler;

class co_scheduler_factory
{
public:
    virtual co_scheduler* create_scheduler()                         = 0;
    virtual void          destroy_scheduler(co_scheduler* scheduler) = 0;
};