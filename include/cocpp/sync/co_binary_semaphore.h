_Pragma("once");

#include "cocpp/sync/co_counting_semaphore.h"

CO_NAMESPACE_BEGIN

using co_binary_semaphore = co_counting_semaphore<1>; // 二进制信号量

CO_NAMESPACE_END