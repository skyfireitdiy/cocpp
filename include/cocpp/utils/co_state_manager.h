_Pragma("once");

#include "cocpp/core/co_define.h"
#include "cocpp/sync/co_spinlock.h"

#include <mutex>

CO_NAMESPACE_BEGIN

template <typename T, T InitState, T FinalState>
class co_state_manager final
{
private:
    mutable std::mutex mu_state__;
    T                  state__ { InitState };

public:
    void set_state(const T& state);
    T    state() const;
};

/////////////////////////////////// 模板实现 /////////////////////////////
template <typename T, T InitState, T FinalState>
void co_state_manager<T, InitState, FinalState>::set_state(const T& state)
{
    std::scoped_lock lock(mu_state__);
    if (state__ != FinalState)
    {
        state__ = state;
    }
}

template <typename T, T InitState, T FinalState>
T co_state_manager<T, InitState, FinalState>::state() const
{
    std::scoped_lock lock(mu_state__);
    return state__;
}

CO_NAMESPACE_END