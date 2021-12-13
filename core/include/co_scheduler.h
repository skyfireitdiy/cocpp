_Pragma("once");

#include <list>

#include "co_define.h"
#include "co_noncopyable.h"

CO_NAMESPACE_BEGIN

class co_ctx;

class co_scheduler : private co_noncopyable
{
public:
    virtual void               add_ctx(co_ctx* ctx)                  = 0;
    virtual void               remove_ctx(co_ctx* ctx)               = 0;
    virtual co_ctx*            choose_ctx()                          = 0;
    virtual std::list<co_ctx*> all_ctx() const                       = 0;
    virtual std::list<co_ctx*> all_scheduleable_ctx() const          = 0;
    virtual size_t             count() const                         = 0;
    virtual co_ctx*            current_ctx() const                   = 0;
    virtual bool               can_schedule() const                  = 0;
    virtual void               change_priority(int old, co_ctx* ctx) = 0;
    virtual void               ctx_leave_wait_state(co_ctx* ctx)     = 0;
    virtual void               ctx_enter_wait_state(co_ctx* ctx)     = 0;
    virtual co_ctx*            take_one_movable_ctx()                = 0;
    virtual std::list<co_ctx*> take_all_movable_ctx()                = 0;

    virtual ~co_scheduler() = default;
};

CO_NAMESPACE_END