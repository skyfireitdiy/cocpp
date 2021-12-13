_Pragma("once");

#include "co_define.h"
#include "co_scheduler.h"
#include "co_spinlock.h"

#include <list>
#include <unordered_set>
#include <vector>

CO_NAMESPACE_BEGIN

class co_o1_scheduler : public co_scheduler
{
private:
    std::vector<std::list<co_ctx*>> all_scheduleable_ctx__;
    std::unordered_set<co_ctx*>     blocked_ctx__;
    mutable co_spinlock             mu_scheduleable_ctx__ { co_spinlock::lock_type::in_thread };
    mutable co_spinlock             mu_blocked_ctx__ { co_spinlock::lock_type::in_thread };
    co_ctx*                         curr_obj__ { nullptr };
    int                             min_priority__ = 0;

    void update_min_priority__(int priority);

    co_o1_scheduler();

public:
    void               add_ctx(co_ctx* ctx) override;
    void               remove_ctx(co_ctx* ctx) override;
    co_ctx*            choose_ctx() override;
    std::list<co_ctx*> all_ctx() const override;
    std::list<co_ctx*> all_scheduleable_ctx() const override;
    size_t             count() const override;
    co_ctx*            current_ctx() const override;
    bool               can_schedule() const override;
    void               change_priority(int old, co_ctx* ctx) override;
    void               ctx_leave_wait_state(co_ctx* ctx) override;
    void               ctx_enter_wait_state(co_ctx* ctx) override;
    co_ctx*            take_one_movable_ctx() override;
    std::list<co_ctx*> take_all_movable_ctx() override;

    friend class co_o1_scheduler_factory;
};

CO_NAMESPACE_END