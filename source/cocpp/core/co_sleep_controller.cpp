#include "cocpp/core/co_sleep_controller.h"
#include "cocpp/core/co_define.h"

CO_NAMESPACE_BEGIN

void co_sleep_controller::wake_up()
{
    std::lock_guard<std::mutex> lock(mu__);
    cond__.notify_one();
}

void co_sleep_controller::sleep_if_need()
{
    std::unique_lock<std::mutex> lock(mu__);
    while (checker__())
    {
        cond__.wait(lock);
    }
}

void co_sleep_controller::sleep_if_need(std::function<bool()> checker)
{
    std::unique_lock<std::mutex> lock(mu__);
    while (checker())
    {
        cond__.wait(lock);
    }
}

co_sleep_controller::co_sleep_controller(std::function<bool()> checker)
    : checker__(checker)
{
}

std::mutex& co_sleep_controller::sleep_lock()
{
    return mu__;
}

CO_NAMESPACE_END