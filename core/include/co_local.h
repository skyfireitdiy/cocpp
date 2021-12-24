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
    T local_data__; // 局部数据
public:             //
    T& get();       // 获取局部数据
};

// 模板实现
template <typename T>
T& co_local<T>::get()
{
    return local_data__;
}

CO_NAMESPACE_END