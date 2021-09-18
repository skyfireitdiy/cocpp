#pragma once
#include <iostream>
#include <thread>

// 默认栈大小
constexpr size_t CO_DEFAULT_STACK_SIZE = 1024 * 1024 * 8;

// 协程flag
constexpr int CO_CTX_FLAG_WAITING      = 0; // 被等待
constexpr int CO_CTX_FLAG_HANDLE_BY_CO = 1; // 被co对象持有，暂时不能销毁
constexpr int CO_CTX_FLAG_MAX          = 8;

// 协程优先级
constexpr int CO_IDLE_CTX_PRIORITY = 99;
constexpr int CO_MAX_PRIORITY      = 100;

// 调试宏
#define CO_OUTPUT(level, fmt, ...)                         \
    {                                                      \
        printf("[%s] %s(%u) %s :[0x%x] -> " fmt "\n",      \
               level, __FILE__, __LINE__, __FUNCTION__,    \
               std::this_thread::get_id(), ##__VA_ARGS__); \
    }

#ifdef NDEBUG
#define CO_DEBUG(...)
#else
#define CO_DEBUG(fmt, ...) CO_OUTPUT("DEBUG", fmt, ##__VA_ARGS__)
#endif
