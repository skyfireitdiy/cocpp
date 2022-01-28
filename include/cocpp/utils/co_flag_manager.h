_Pragma("once");

#include "cocpp/core/co_define.h"
#include "cocpp/sync/co_spinlock.h"

#include <bitset>
#include <mutex>

CO_NAMESPACE_BEGIN

template <size_t MAX_FLAG_COUNT>
class co_flag_manager final
{
private:
    std::bitset<MAX_FLAG_COUNT>  flags__;
    mutable std::recursive_mutex mu__;

public:
    void set_flag(size_t flag)
    {
        std::scoped_lock lock(mu__);
        flags__.set(flag);
    }
    void reset_flag(size_t flag)
    {
        std::scoped_lock lock(mu__);
        flags__.reset(flag);
    }
    bool test_flag(size_t flag) const
    {
        std::scoped_lock lock(mu__);
        return flags__.test(flag);
    }
};

CO_NAMESPACE_END