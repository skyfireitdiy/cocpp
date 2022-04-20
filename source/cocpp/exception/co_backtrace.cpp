#include "cocpp/exception/co_backtrace.h"
#include "cocpp/core/co_define.h"
#include <execinfo.h>
#include <unistd.h>

using namespace std;

CO_NAMESPACE_BEGIN

vector<string> backtrace()
{
    vector<void*> buffer(100);
    int           pointer_num   = ::backtrace(buffer.data(), buffer.size());
    char**        string_buffer = ::backtrace_symbols(buffer.data(), pointer_num);
    if (string_buffer == NULL)
    {
        return {};
    }
    vector<string> ret(pointer_num);
    for (int i = 0; i < pointer_num; i++)
    {
        ret[i] = string_buffer[i];
    }
    free(string_buffer);
    return ret;
}

void print_backtrace()
{
    auto bt = backtrace();
    for (auto& b : bt)
    {
        printf("%s\n", b.c_str());
    }
}

CO_NAMESPACE_END