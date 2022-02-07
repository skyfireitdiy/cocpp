_Pragma("once");

#include "cocpp/core/co_define.h"
#include "cocpp/interface/co.h"

CO_NAMESPACE_BEGIN

template <typename Func, typename... Args>
auto co_async_run(Func&& func, Args&&... args);
template <typename Func, typename... Args>
auto co_async_run(std::initializer_list<std::function<void(co_ctx_config&)>> opts, Func&& func, Args&&... args);

// 模板实现

template <typename Func, typename... Args>
auto co_async_run(Func&& func, Args&&... args)
{
    return co_async_run({}, std::forward<Func>(func), std::forward<Args>(args)...);
}

template <typename Func, typename... Args>
auto co_async_run(std::initializer_list<std::function<void(co_ctx_config&)>> opts, Func&& func, Args&&... args)
{
    auto task   = std::make_shared<std::packaged_task<std::invoke_result_t<std::decay_t<Func>, std::decay_t<Args>...>()>>(std::bind(std::forward<Func>(func), std::forward<Args>(args)...));
    auto future = task->get_future();
    co(opts, [task]() { (*task)(); }).detach();
    return future;
}

CO_NAMESPACE_END