#pragma once

#include "co_mutex.h"
#include "co_timed_addition.h"

using co_timed_mutex = co_timed_addition<co_mutex>;