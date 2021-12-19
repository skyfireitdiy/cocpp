_Pragma("once");

#include "co_define.h"
#include "co_singleton.h"
#include "co_spinlock.h"

#include <atomic>
#include <list>
#include <vector>

CO_NAMESPACE_BEGIN

class co_ctx;

class co_scheduler final : public co_singleton<co_scheduler>
{
private:
    std::vector<std::list<co_ctx*>> all_normal_ctx__ { CO_MAX_PRIORITY };
    size_t                          count__ { 0 };
    size_t                          min_priority__ { 0 };
    co_spinlock                     all_normal_ctx_lock__;

public:
    void               add_ctx(co_ctx* ctx);
    co_ctx*            choose_next_ctx();
    size_t             min_priority();
    size_t             count();
    std::list<co_ctx*> all_ctx();
};

CO_NAMESPACE_END