_Pragma("once");

#include "co_singleton.h"

CO_NAMESPACE_BEGIN

class co_scheduler;

class co_scheduler_factory final : public co_singleton<co_scheduler_factory>
{
public:
    co_scheduler* create_scheduler();
    void          destroy_scheduler(co_scheduler* scheduler);
};

CO_NAMESPACE_END