_Pragma("once");

#include "cocpp/core/co_define.h"

#include <condition_variable>
#include <functional>
#include <mutex>

CO_NAMESPACE_BEGIN

class co_sleep_controller final
{
private:
    std::recursive_mutex        mu__;
    std::condition_variable_any cv__;
    std::function<bool()>       checker__;

public:
    co_sleep_controller(std::function<bool()> checker);
    void                  wake_up();
    void                  sleep_if_need();
    void                  sleep_if_need(std::function<bool()> checker);
    std::recursive_mutex& sleep_lock();
};

CO_NAMESPACE_END