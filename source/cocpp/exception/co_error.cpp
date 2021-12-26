#include "cocpp/exception/co_error.h"

CO_NAMESPACE_BEGIN

const char* co_error::what() const noexcept
{
    return what__.c_str();
}

CO_NAMESPACE_END