#include "cocpp/core/co_define.h"
#include <chrono>
#include <functional>

CO_NAMESPACE_BEGIN
bool co_timed_call(std::chrono::steady_clock::duration timeout, std::function<bool()> func);
CO_NAMESPACE_END
