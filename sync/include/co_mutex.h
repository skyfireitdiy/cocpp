_Pragma("once");

#include "co_ctx.h"
#include "co_define.h"
#include "co_noncopyable.h"
#include "co_spinlock.h"
#include <atomic>
#include <list>

CO_NAMESPACE_BEGIN

using co_mutex = co_spinlock;

CO_NAMESPACE_END