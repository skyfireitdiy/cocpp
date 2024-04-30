_Pragma("once");

#include "cocpp/core/co_define.h"
#include <string>
#include <any>

CO_NAMESPACE_BEGIN

enum class co_error_code
{
    ok = 0,
}

class co_error final
{
private:
    co_error_code code__;
    std::string msg__;
    std::any data__;

public:
    co_error(int code = co_error_code::ok,
             const std::string &msg = "",
             const std::any &data = nullptr)
        : code__(code)
        , msg__(msg)
        , data__(data)
    {
    }
    int code() const
    {
        return code__;
    }
    const std::string &msg() const
    {
        return msg__;
    }
    const std::any &data() const
    {
        return data__;
    }
    operator bool() const
    {
        return code__ != co_error_code::ok;
    }
};

CO_NAMESPACE_END
