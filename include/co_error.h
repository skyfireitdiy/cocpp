#pragma once

#include <exception>
#include <sstream>
#include <string>

#include "co_define.h"

CO_NAMESPACE_BEGIN
class co_error : public std::exception
{
    std::string what__;

public:
    template <typename... Args>
    co_error(Args&&... args);
};

template <typename... Args>
co_error::co_error(Args&&... args)
{
    std::ostringstream so;
    (so << ... << args);
}

CO_NAMESPACE_END