#pragma once
#include <iostream>
#include <thread>

#include <unistd.h>

#define CO_NAMESPACE cocpp
#define CO_NAMESPACE_BEGIN \
    namespace CO_NAMESPACE \
    {

#define CO_NAMESPACE_END }

CO_NAMESPACE_BEGIN
// 默认栈大小
constexpr size_t        CO_DEFAULT_STACK_SIZE = 1024 * 1024 * 8;
constexpr static size_t MIN_STACK_MEM_ZONE    = 2;
constexpr static size_t MAX_STACK_ZONE_COUNT  = 24;

// 协程flag
constexpr int CO_CTX_FLAG_WAITING      = 0; // 等待
constexpr int CO_CTX_FLAG_LOCKED       = 1; // 被co对象持有，暂时不能销毁
constexpr int CO_CTX_FLAG_BIND         = 2; // 绑定env，不可移动
constexpr int CO_CTX_FLAG_IDLE         = 3; // idle ctx
constexpr int CO_CTX_FLAG_SHARED_STACK = 4; // 共享栈
constexpr int CO_CTX_FLAG_SWITCHING    = 5; // 正在切换
constexpr int CO_CTX_FLAG_MAX          = 8;

constexpr int CO_ENV_FLAG_NO_SCHE_THREAD    = 0; // 没有调度线程
constexpr int CO_ENV_FLAG_COVERTED          = 1; // 从正常线程转换来的调度线程
constexpr int CO_ENV_FLAG_SCHEDULED         = 2; // 被调度过
constexpr int CO_ENV_FLAG_DONT_AUTO_DESTORY = 3; // 禁止被自动清理线程选中
constexpr int CO_ENV_FLAG_MAX               = 8;

// 协程优先级
constexpr int CO_IDLE_CTX_PRIORITY = 99;
constexpr int CO_MAX_PRIORITY      = 100;

CO_NAMESPACE_END

// 调试宏
#define CO_OUTPUT(level, fmt, ...)                              \
    {                                                           \
        printf("[%s] %s(%u) %s :[0x%x] -> " fmt "\n",           \
               level, __FILE__, __LINE__, __FUNCTION__,         \
               reinterpret_cast<int>(gettid()), ##__VA_ARGS__); \
    }

#define CO_O_OUTPUT(level, fmt, ...)                                  \
    {                                                                 \
        printf("[%s] %s(%u) %s :[0x%x] %p -> " fmt "\n",              \
               level, __FILE__, __LINE__, __FUNCTION__,               \
               reinterpret_cast<int>(gettid()), this, ##__VA_ARGS__); \
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
