_Pragma("once");

#include "co_define.h"
#include "co_recursive_mutex.h"
#include "co_timed_addition.h"

CO_NAMESPACE_BEGIN

using co_recursive_timed_mutex = co_timed_addition<co_recursive_mutex>; // 递归时间互斥锁

CO_NAMESPACE_END