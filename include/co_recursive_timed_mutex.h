#pragma once

#include "co_recursive_mutex.h"
#include "co_timed_addition.h"

using co_recursive_timed_mutex = co_timed_addition<co_recursive_mutex>;