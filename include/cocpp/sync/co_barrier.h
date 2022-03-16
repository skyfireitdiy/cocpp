_Pragma("once");

#include "cocpp/core/co_define.h"
#include "cocpp/sync/co_condition_variable.h"
#include "cocpp/sync/co_mutex.h"
#include "cocpp/utils/co_noncopyable.h"

CO_NAMESPACE_BEGIN

struct co_arrival_token
{
    co_condition_variable condition__;
    co_mutex              mutex__;
    std::ptrdiff_t        count__;
};

class co_barrier final : private co_noncopyable
{
private:
    std::ptrdiff_t              expected__;
    std::atomic<std::ptrdiff_t> count__;
    co_mutex                    mutex__;
    co_condition_variable       cond__;

public:
    explicit co_barrier(std::ptrdiff_t expected);
    co_arrival_token arrive(std::ptrdiff_t n = 1);
    void             wait(co_arrival_token&& arrival) const;
    void             arrive_and_wait();
    void             arrive_and_drop();
};

CO_NAMESPACE_END