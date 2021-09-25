#pragma once

#include <bitset>
#include <mutex>

template <size_t MAX_FLAG_COUNT>
class co_flag_manager
{
private:
    std::bitset<MAX_FLAG_COUNT> flags__;
    mutable std::mutex          mu__;

public:
    void set_flag(size_t flag)
    {
        std::lock_guard<std::mutex> lock(mu__);
        flags__.set(flag);
    }
    void reset_flag(size_t flag)
    {
        std::lock_guard<std::mutex> lock(mu__);
        flags__.reset(flag);
    }
    bool test_flag(size_t flag) const
    {
        std::lock_guard<std::mutex> lock(mu__);
        return flags__.test(flag);
    }
};