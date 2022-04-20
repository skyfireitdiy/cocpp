#include "cocpp/utils/co_utils.h"

using namespace std;

CO_NAMESPACE_BEGIN

bool co_timed_call(chrono::steady_clock::duration timeout, function<bool()> func)
{
    auto deadline = chrono::steady_clock::now() + timeout;
    while (chrono::steady_clock::now() < deadline)
    {
        if (func())
        {
            return true;
        }
    }
    return false;
}

CO_NAMESPACE_END