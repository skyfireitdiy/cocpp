_Pragma("once");

#include "cocpp/core/co_define.h"
#include "cocpp/sync/co_condition_variable.h"
#include "cocpp/sync/co_mutex.h"
#include "cocpp/utils/co_noncopyable.h"

CO_NAMESPACE_BEGIN

class co_wait_group final : private co_noncopyable
{
private:
    const size_t          count__;
    size_t                done__;
    co_mutex              mutex__;
    co_condition_variable cond__;

public:
    explicit co_wait_group(size_t count);
    ~co_wait_group();
    void done();
    void wait();
};

CO_NAMESPACE_END