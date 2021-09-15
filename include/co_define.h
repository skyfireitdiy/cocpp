#pragma once
#include <iostream>

// 默认栈大小
constexpr size_t CO_DEFAULT_STACK_SIZE = 1024 * 1024 * 8;

// 调试宏
#define CO_OUTPUT(level, fmt, ...)                                      \
    {                                                                   \
        printf("[%s] %s(%d) %s -> " fmt "\n",                           \
               level, __FILE__, __LINE__, __FUNCTION__, ##__VA_ARGS__); \
    }

#ifdef NDEBUG
#define CO_DEBUG(...)
#else
#define CO_DEBUG(fmt, ...) CO_OUTPUT("DEBUG", fmt, __VA_ARGS__)
#endif