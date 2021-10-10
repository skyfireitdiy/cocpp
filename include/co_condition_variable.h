#pragma once

#include <condition_variable>

using co_condition_variable = std::condition_variable_any;

template <class Lock>
void notify_all_at_thread_exit(co_condition_variable& cond,
                               Lock&                  lk);

/// 模板实现

template <class Lock>
void notify_all_at_thread_exit(co_condition_variable& cond,
                               Lock&                  lk)
{
}