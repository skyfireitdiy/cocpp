_Pragma("once");
#include "co_define.h"

CO_NAMESPACE_BEGIN
class co_noncopyable
{
private:
    co_noncopyable(const co_noncopyable&) = delete;
    co_noncopyable(co_noncopyable&&)      = delete;
    co_noncopyable& operator=(const co_noncopyable&) = delete;
    co_noncopyable& operator=(co_noncopyable&&) = delete;

public:
    co_noncopyable()  = default;
    ~co_noncopyable() = default;
};

CO_NAMESPACE_END