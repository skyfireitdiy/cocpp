_Pragma("once");

#include "co_define.h"

#include <condition_variable>
#include <functional>
#include <mutex>

CO_NAMESPACE_BEGIN

class co_sleep_controller final
{
private:
    std::mutex              mu__;
    std::condition_variable cond__;
    std::function<bool()>   checker__;

public:
    co_sleep_controller(std::function<bool()> checker);
    void wake_up();
    void sleep_if_need();
};

CO_NAMESPACE_END