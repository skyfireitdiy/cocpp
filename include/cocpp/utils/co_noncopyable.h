_Pragma("once");
#include "cocpp/core/co_define.h"

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

class co_noncopyable_with_move
{
private:
    co_noncopyable_with_move(const co_noncopyable_with_move&) = delete;
    co_noncopyable_with_move& operator=(const co_noncopyable_with_move&) = delete;

public:
    co_noncopyable_with_move()  = default;
    ~co_noncopyable_with_move() = default;
};

CO_NAMESPACE_END