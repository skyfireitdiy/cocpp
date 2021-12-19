_Pragma("once");

#include "co_define.h"
#include "co_noncopyable.h"
#include "co_scheduler.h"
#include "co_spinlock.h"

#include <list>
#include <unordered_set>
#include <vector>

CO_NAMESPACE_BEGIN

class co_ctx;
class co_scheduler final : private co_noncopyable
{
private:
    std::vector<std::list<co_ctx*>> all_scheduleable_ctx__;
    std::unordered_set<co_ctx*>     blocked_ctx__;
    mutable co_spinlock             mu_scheduleable_ctx__ { co_spinlock::lock_type::in_thread };
    mutable co_spinlock             mu_blocked_ctx__ { co_spinlock::lock_type::in_thread };
    co_ctx*                         curr_obj__ { nullptr };
    int                             min_priority__ = 0;

    void update_min_priority__(int priority);

    co_scheduler();

public:
    // void               add_ctx(co_ctx* ctx);
    // void               remove_ctx(co_ctx* ctx);
    // co_ctx*            choose_ctx();
    // std::list<co_ctx*> all_ctx() const;
    // std::list<co_ctx*> all_scheduleable_ctx() const;
    // size_t             count() const;
    // co_ctx*            current_ctx() const;
    // bool               can_schedule() const;
    // void               change_priority(int old, co_ctx* ctx);
    // void               ctx_leave_wait_state(co_ctx* ctx);
    void               ctx_enter_wait_state(co_ctx* ctx);
    std::list<co_ctx*> take_all_movable_ctx();
    co_ctx*            take_one_movable_ctx();

    friend class co_scheduler_factory;
    friend class co_env;
};

CO_NAMESPACE_END