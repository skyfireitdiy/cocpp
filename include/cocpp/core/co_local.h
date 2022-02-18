_Pragma("once");

#include "cocpp/core/co_define.h"

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
    T local_data__;

public:
    T& get();
};

template <typename T>
T& co_local<T>::get()
{
    return local_data__;
}

CO_NAMESPACE_END