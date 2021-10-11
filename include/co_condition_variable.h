#pragma once

#include "co.h"
#include "co_define.h"
#include <condition_variable>

CO_NAMESPACE_BEGIN

using co_condition_variable = std::condition_variable_any;

void notify_all_at_co_exit(co_condition_variable& cond);

CO_NAMESPACE_END