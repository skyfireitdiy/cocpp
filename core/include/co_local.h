_Pragma("once");

#include "co_define.h"

CO_NAMESPACE_BEGIN

class co_local_base
{
public:
    virtual ~co_local_base() = default;
};

template <typename T>
class co_local : public co_local_base
{
private:
    T data__;

public:
    T& get()
    {
        return data__;
    }
};

CO_NAMESPACE_END