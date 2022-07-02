_Pragma("once");

#include <cstddef>
#include <unistd.h>

#define CO_NAMESPACE cocpp
#define CO_NAMESPACE_BEGIN \
    namespace CO_NAMESPACE \
    {

#define CO_NAMESPACE_END }

CO_NAMESPACE_BEGIN

constexpr size_t CO_DEFAULT_STACK_SIZE = 1024 * 1024 * 8;

constexpr static size_t MAX_STACK_CACHE_COUNT = 1024;

constexpr static size_t MAX_CTX_CACHE_COUNT = 1024;

constexpr static size_t MAX_ENV_CACHE_COUNT = 1024;

constexpr static size_t DEFAULT_TIMING_TICK_DURATION_IN_MS = 10;

constexpr static size_t TICKS_COUNT_OF_FREE_MEM = 1000;

constexpr int CO_CTX_FLAG_WAITING      = 0;
constexpr int CO_CTX_FLAG_LOCKED       = 1;
constexpr int CO_CTX_FLAG_BIND         = 2;
constexpr int CO_CTX_FLAG_IDLE         = 3;
constexpr int CO_CTX_FLAG_SHARED_STACK = 4;
constexpr int CO_CTX_FLAG_MAX          = 8;

constexpr int CO_ENV_FLAG_NO_SCHE_THREAD    = 0;
constexpr int CO_ENV_FLAG_CONVERTED         = 1;
constexpr int CO_ENV_FLAG_SCHEDULED         = 2;
constexpr int CO_ENV_FLAG_DONT_AUTO_DESTROY = 3;
constexpr int CO_ENV_FLAG_EXCLUSIVE         = 4;
constexpr int CO_ENV_FLAG_MAX               = 8;

constexpr int CO_IDLE_CTX_PRIORITY = 99;
constexpr int CO_MAX_PRIORITY      = 100;

constexpr int CO_SWITCH_SIGNAL = 10;

CO_NAMESPACE_END

#define CO_OUTPUT(level, fmt, ...)                                                     \
    {                                                                                  \
        fprintf(stderr, "[%s] %s(%u) %s :[0x%llx] -> " fmt "\n",                       \
                level, __FILE__, __LINE__, __FUNCTION__,                               \
                static_cast<unsigned long long>(gettid()) __VA_OPT__(, ) __VA_ARGS__); \
    }

#define CO_O_OUTPUT(level, fmt, ...)                                                         \
    {                                                                                        \
        fprintf(stderr, "[%s] %s(%u) %s :[0x%llx] %p -> " fmt "\n",                          \
                level, __FILE__, __LINE__, __FUNCTION__,                                     \
                static_cast<unsigned long long>(gettid()), this __VA_OPT__(, ) __VA_ARGS__); \
    }

#ifdef NDEBUG
#define CO_DEBUG(...)
#define CO_WARN(...)
#define CO_ERROR(...)

#define CO_O_DEBUG(...)
#define CO_O_WARN(...)
#define CO_O_ERROR(...)

#else
#define CO_DEBUG(fmt, ...) CO_OUTPUT("DEBUG", fmt, ##__VA_ARGS__)
#define CO_WARN(fmt, ...) CO_OUTPUT("WARN ", fmt, ##__VA_ARGS__)
#define CO_ERROR(fmt, ...) CO_OUTPUT("ERROR", fmt, ##__VA_ARGS__)

#define CO_O_DEBUG(fmt, ...) CO_O_OUTPUT("DEBUG", fmt, ##__VA_ARGS__)
#define CO_O_WARN(fmt, ...) CO_O_OUTPUT("WARN ", fmt, ##__VA_ARGS__)
#define CO_O_ERROR(fmt, ...) CO_O_OUTPUT("ERROR", fmt, ##__VA_ARGS__)
#endif

#define CoMemberMethodProxy(member, method)                   \
    template <typename... Args>                               \
    decltype(auto) method(Args&&... args)                     \
    {                                                         \
        return (member)->method(std::forward<Args>(args)...); \
    }

#define CoConstMemberMethodProxy(member, method)              \
    template <typename... Args>                               \
    decltype(auto) method(Args&&... args) const               \
    {                                                         \
        return (member)->method(std::forward<Args>(args)...); \
    }

#define CoMemberMethodProxyStatic(member, method)             \
    template <typename... Args>                               \
    static decltype(auto) method(Args&&... args)              \
    {                                                         \
        return (member)->method(std::forward<Args>(args)...); \
    }

#define CoMemberMethodProxyWithPrefix(member, method, prefix) \
    template <typename... Args>                               \
    decltype(auto) prefix##method(Args&&... args)             \
    {                                                         \
        return (member)->method(std::forward<Args>(args)...); \
    }

#define CoConstMemberMethodProxyWithPrefix(member, method, prefix) \
    template <typename... Args>                                    \
    decltype(auto) prefix##method(Args&&... args) const            \
    {                                                              \
        return (member)->method(std::forward<Args>(args)...);      \
    }

#define CoMemberMethodProxyStaticWithPrefix(member, method, prefix) \
    template <typename... Args>                                     \
    static decltype(auto) prefix##method(Args&&... args)            \
    {                                                               \
        return (member)->method(std::forward<Args>(args)...);       \
    }

#define CoCurrentEnv() co_manager::instance()->current_env()
#define CoCurrentCtx() CoCurrentEnv()->current_ctx()

#define CoYield() CoCurrentEnv()->schedule_switch()

#define CoScheduleGuard() co_schedule_guard __CoScheduleGuard__(CoCurrentEnv()->schedule_lock())
#define CoLockSchedule() CoCurrentEnv()->schedule_lock().lock()
#define CoUnlockSchedule() CoCurrentEnv()->schedule_lock().unlock()

#define CoPreemptGuard() co_preempt_guard __CoPreemptGuard__(CoCurrentEnv()->schedule_lock())
#define CoEnablePreempt() CoCurrentEnv()->schedule_lock().decreate_count()
#define CoDisablePreempt() CoCurrentEnv()->schedule_lock().increate_count()
