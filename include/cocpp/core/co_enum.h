_Pragma("once");

#include "cocpp/core/co_define.h"
#include <any>
#include <functional>

CO_NAMESPACE_BEGIN

#define ENUM_ITEM(Name)                                          \
    template <typename... DataList>                              \
    struct Name                                                  \
    {                                                            \
    private:                                                     \
        std::tuple<DataList...> data;                            \
                                                                 \
    public:                                                      \
        Name(DataList... data_list)                              \
            : data(std::make_tuple(data_list...))                \
        {                                                        \
        }                                                        \
        void apply(std::function<void(DataList... data_list)> f) \
        {                                                        \
            std::apply(f, data);                                 \
        }                                                        \
    };                                                           \
                                                                 \
    template <typename... DataList>                              \
    Name(DataList...) -> Name<DataList...>;

#define ENUM_TYPE(TYPE)                                                \
    class TYPE                                                         \
    {                                                                  \
    private:                                                           \
        std::any data__;                                               \
                                                                       \
    public:                                                            \
        template <template <typename...> typename U, typename... Item> \
        TYPE(const U<Item...>& item)                                   \
            : data__(item)                                             \
        {                                                              \
        }                                                              \
        template <template <typename...> typename T, typename... Args> \
        TYPE& $(std::function<void(Args...)> f)                        \
        {                                                              \
            if (data__.type() == typeid(T<Args...>))                   \
            {                                                          \
                std::any_cast<T<Args...>>(data__).apply(f);            \
            }                                                          \
            return *this;                                              \
        }                                                              \
        template <template <typename...> typename T, typename U>       \
        TYPE& $(U f)                                                   \
        {                                                              \
            return $<T>(std::function(f));                             \
        }                                                              \
    };

CO_NAMESPACE_END