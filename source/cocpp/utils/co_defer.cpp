#include "cocpp/utils/co_defer.h"
#include <functional>

CO_NAMESPACE_BEGIN

co_defer::co_defer(std::function<void()> f)
    : defer_func__(f)
{
}

co_defer::~co_defer()
{
    defer_func__();
}

CO_NAMESPACE_END