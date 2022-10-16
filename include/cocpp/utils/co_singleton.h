_Pragma("once");
#include <utility>

#include "cocpp/core/co_define.h"
#include "cocpp/utils/co_noncopyable.h"

CO_NAMESPACE_BEGIN

template <typename T>
class co_singleton : private co_noncopyable
{
private:
    co_singleton() = default;

public:
    static T *instance();
    static void destroy_instance();
    friend T;
};

template <typename T>
class co_singleton_static : private co_noncopyable
{
private:
    co_singleton_static() = default;

public:
    static T *instance();
    friend T;
};

template <typename T>
T *co_singleton<T>::instance()
{
    static T *inst = new T;
    return inst;
}

template <typename T>
void co_singleton<T>::destroy_instance()
{
    delete instance();
}

template <typename T>
T *co_singleton_static<T>::instance()
{
    static T inst;
    return &inst;
}

CO_NAMESPACE_END