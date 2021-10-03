#pragma once

#include <exception>
#include <sstream>
#include <string>

class co_error : public std::exception
{
    std::string what__;

public:
    template <typename... Args>
    co_error(Args&&... args)
    {
        std::ostringstream so;
        (so << ... << args);
    }
};