_Pragma("once");

#include "co_define.h"

CO_NAMESPACE_BEGIN

class co_scheduler;
class co_manager;

class co_scheduler_factory
{
public:
    virtual co_scheduler* create_scheduler()                         = 0;
    virtual void          destroy_scheduler(co_scheduler* scheduler) = 0;
    virtual ~co_scheduler_factory()                                  = default;
};

CO_NAMESPACE_END