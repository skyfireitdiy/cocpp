_Pragma("once");

#include <list>

#include "co_define.h"
#include "co_noncopyable.h"

CO_NAMESPACE_BEGIN

class co_ctx;

class co_scheduler : private co_noncopyable
{
public:
    virtual void               add_obj(co_ctx* obj)                  = 0;
    virtual void               remove_obj(co_ctx* obj)               = 0;
    virtual co_ctx*            choose_obj()                          = 0;
    virtual std::list<co_ctx*> all_obj() const                       = 0;
    virtual size_t             count() const                         = 0;
    virtual co_ctx*            current_obj() const                   = 0;
    virtual bool               can_schedule() const                  = 0;
    virtual void               change_priority(int old, co_ctx* obj) = 0;

    virtual ~co_scheduler() = default;
};

CO_NAMESPACE_END