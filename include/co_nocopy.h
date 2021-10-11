#pragma once
#include "co_define.h"

CO_NAMESPACE_BEGIN
class co_nocopy
{
private:
    co_nocopy(const co_nocopy&) = delete;
    co_nocopy(co_nocopy&&)      = delete;
    co_nocopy& operator=(const co_nocopy&) = delete;
    co_nocopy& operator=(co_nocopy&&) = delete;

public:
    co_nocopy()  = default;
    ~co_nocopy() = default;
};

CO_NAMESPACE_END