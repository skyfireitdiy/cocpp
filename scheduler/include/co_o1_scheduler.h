_Pragma("once");

#include "co_define.h"
#include "co_scheduler.h"

#include <list>
#include <mutex>
#include <vector>

CO_NAMESPACE_BEGIN

class co_o1_scheduler : public co_scheduler
{
private:
    std::vector<std::list<co_ctx*>> all_ctx__;
    mutable std::mutex              mu_all_ctx__;
    co_ctx*                         curr_obj__ { nullptr };
    int                             min_priority__ = 0;

    void update_min_priority__(int priority);

    co_o1_scheduler();

public:
    void               add_ctx(co_ctx* ctx) override;
    void               remove_ctx(co_ctx* ctx) override;
    co_ctx*            choose_ctx() override;
    std::list<co_ctx*> all_ctx() const override;
    size_t             count() const override;
    co_ctx*            current_ctx() const override;
    bool               can_schedule() const override;
    void               change_priority(int old, co_ctx* ctx) override;
    void               ctx_wake_up(co_ctx* ctx) override;

    friend class co_o1_scheduler_factory;
};

CO_NAMESPACE_END