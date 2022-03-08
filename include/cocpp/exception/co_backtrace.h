_Pragma("once");

#include "cocpp/core/co_define.h"

#include <vector>
#include <string>


CO_NAMESPACE_BEGIN

std::vector<std::string> backtrace();
void print_backtrace();

CO_NAMESPACE_END