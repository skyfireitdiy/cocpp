_Pragma("once");

#include <exception>
#include <sstream>
#include <string>

#include "cocpp/core/co_define.h"

CO_NAMESPACE_BEGIN
class co_error : public std::exception
{
    std::string what__;

public:
    template <typename... Args>
    co_error(Args&&... args);
    const char* what() const noexcept override;
};

template <typename T>
std::string make_error_str__(const T& data)
{
    std::ostringstream so;
    so << data << " ";
    return so.str();
}

template <typename... Args>
co_error::co_error(Args&&... args)
{
    what__ = (std::string {} + ... + make_error_str__(args));
}

CO_NAMESPACE_END