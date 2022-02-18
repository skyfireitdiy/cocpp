_Pragma("once");

#include "cocpp/core/co_define.h"
#include "cocpp/utils/co_noncopyable.h"

#include <memory>

CO_NAMESPACE_BEGIN

class co_any final 
{
private:
    class base_type
    {
    public:
        virtual ~base_type() = default;
    };

    template <typename T>
    class real_type : public base_type
    {
        T value__;

    public:
        real_type(const T& value);
        T& get();
    };

    std::shared_ptr<base_type> data__ { nullptr };

public:
    co_any()                    = default;
    co_any(const co_any& value) = default;
    co_any& operator=(const co_any& value) = default;
    template <typename T>
    co_any(const T& value);
    template <typename T>
    co_any& operator=(const T& other);
    template <typename T>
    T& get();
};



template <typename T>
T& co_any::real_type<T>::get()
{
    return value__;
}

template <typename T>
co_any::real_type<T>::real_type(const T& value)
    : value__(value)
{
}

template <typename T>
co_any::co_any(const T& value)
    : data__(std::shared_ptr<real_type<T>>(new real_type<T>(value)))
{
}

template <typename T>
co_any& co_any::operator=(const T& value)
{
    data__ = std::shared_ptr<real_type<T>>(new real_type<T>(value));
    return *this;
}

template <typename T>
T& co_any::get()
{
    auto ptr = std::dynamic_pointer_cast<real_type<T>>(data__);
    if (ptr == nullptr)
    {
        throw std::bad_cast();
    }
    return ptr->get();
}

CO_NAMESPACE_END