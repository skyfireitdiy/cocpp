_Pragma("once");

#include "cocpp/core/co_define.h"
#include "cocpp/sync/co_mutex.h"
#include "cocpp/sync/co_timed_addition.h"

CO_NAMESPACE_BEGIN

using co_timed_mutex = co_timed_addition<co_mutex>; 

CO_NAMESPACE_END