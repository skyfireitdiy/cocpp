#include <atomic>
_Pragma("once");

#include "cocpp/core/co_define.h"
#include "cocpp/sync/co_condition_variable.h"
#include "cocpp/sync/co_mutex.h"
#include "cocpp/utils/co_noncopyable.h"

#include <atomic>

CO_NAMESPACE_BEGIN

class co_latch final : private co_noncopyable
{
private:
    std::ptrdiff_t                expect__;
    mutable co_mutex              mutex__;
    mutable co_condition_variable cond__;

public:
    explicit co_latch(std::ptrdiff_t expected);
    void count_down(std::ptrdiff_t n = 1);
    bool try_wait() const noexcept;
    void wait() const;
    void arrive_and_wait(std::ptrdiff_t n = 1);
};

CO_NAMESPACE_END