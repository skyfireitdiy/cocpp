#pragma once

#include "co_define.h"

#include <bitset>
#include <mutex>

CO_NAMESPACE_BEGIN

template <size_t MAX_FLAG_COUNT>
class co_flag_manager
{
private:
    std::bitset<MAX_FLAG_COUNT> flags__;
    mutable co_spinlock         mu__;

public:
    void set_flag(size_t flag)
    {
        std::lock_guard<co_spinlock> lock(mu__);
        flags__.set(flag);
    }
    void reset_flag(size_t flag)
    {
        std::lock_guard<co_spinlock> lock(mu__);
        flags__.reset(flag);
    }
    bool test_flag(size_t flag) const
    {
        std::lock_guard<co_spinlock> lock(mu__);
        return flags__.test(flag);
    }
};

CO_NAMESPACE_END