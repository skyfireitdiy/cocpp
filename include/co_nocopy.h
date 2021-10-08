#pragma once

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