_Pragma("once");

#include "cocpp/utils/co_defer.h"
#include "cocpp/utils/co_noncopyable.h"
#include <functional>

CO_NAMESPACE_BEGIN

class co_defer final : public cocpp::co_noncopyable
{
private:
    std::function<void()> defer_func__;

public:
    co_defer(std::function<void()> f);
    ~co_defer();
};

CO_NAMESPACE_END

#define CO_DERFER_HELPER2(line, ...) cocpp::co_defer defer_##line(__VA_ARGS__)
#define CO_DERFER_HELPER1(line, ...) CO_DERFER_HELPER2(line, __VA_ARGS__)
#define CoDefer(...) CO_DERFER_HELPER1(__LINE__, __VA_ARGS__)