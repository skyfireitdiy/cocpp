_Pragma("once");

#include "co_define.h"

#include <functional>
#include <list>

CO_NAMESPACE_BEGIN

class co_ctx;
class co_spinlock;

void lock_yield__(co_ctx* ctx, co_spinlock& lk, std::function<bool()> checker);

CO_NAMESPACE_END