#include "cocpp/core/co_sleep_controller.h"
#include "cocpp/core/co_define.h"

CO_NAMESPACE_BEGIN

void co_sleep_controller::wake_up()
{
    std::scoped_lock lock(mu__);
    cv__.notify_one();
}

void co_sleep_controller::sleep_if_need()
{
    std::unique_lock lock(mu__);
    while (checker__())
    {
        cv__.wait(lock);
    }
}

void co_sleep_controller::sleep_if_need(std::function<bool()> checker)
{
    std::unique_lock lock(mu__);
    while (checker())
    {
        cv__.wait(lock);
    }
}

co_sleep_controller::co_sleep_controller(std::function<bool()> checker)
    : checker__(checker)
{
}

std::recursive_mutex& co_sleep_controller::sleep_lock()
{
    return mu__;
}

CO_NAMESPACE_END