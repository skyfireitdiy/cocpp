#include "cocpp/utils/co_utils.h"

CO_NAMESPACE_BEGIN

bool co_timed_call(std::chrono::steady_clock::duration timeout, std::function<bool()> func)
{
    auto deadline = std::chrono::steady_clock::now() + timeout;
    while (std::chrono::steady_clock::now() < deadline)
    {
        if (func())
        {
            return true;
        }
    }
    return false;
}

CO_NAMESPACE_END