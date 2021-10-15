#pragma once

#include "co_define.h"
#include "co_mutex.h"
#include "co_timed_addition.h"

CO_NAMESPACE_BEGIN

using co_timed_mutex = co_timed_addition<co_mutex>;

CO_NAMESPACE_END