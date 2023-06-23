_Pragma("once");
#include "cocpp/core/co_define.h"
#include <concepts>
#include <optional>

CO_NAMESPACE_BEGIN

namespace pipeline
{

template <typename CollectionType>
concept Iterable = requires(CollectionType c)
{
    c.begin();
    c.end();
};

template <typename FuncType, typename ItemType>
concept PipelineInitFunc = requires(FuncType f, ItemType i)
{
    requires std::invocable<FuncType>;
    requires std::same_as<std::invoke_result_t<FuncType>, std::optional<ItemType> >;
};

template <typename ItemType, typename CollectionType>
concept To = std::is_same_v<std::decay_t<decltype(*std::declval<CollectionType>().begin())>, ItemType>;

template <typename CollectionType>
concept Collection = requires(CollectionType c)
{
    c.begin();
    c.end();
    c.insert(c.begin(), *c.begin());
};

template <typename FilterType, typename ItemType>
concept FilterFunc = requires(FilterType f, ItemType i)
{
    requires std::invocable<FilterType, ItemType>;
    requires std::same_as<std::invoke_result_t<FilterType, ItemType>,
                          bool>;
};

template <typename ReduceType, typename InitType, typename ItemType>
concept ReduceFunc = requires(ReduceType r, InitType i, ItemType it)
{
    requires std::invocable<ReduceType, InitType, ItemType>;
    requires std::same_as<std::invoke_result_t<ReduceType, InitType, ItemType>, InitType>;
};

template <typename FuncType, typename ItemType>
concept ReturnIsNotVoid = requires(FuncType f, ItemType i)
{
    requires std::invocable<FuncType, ItemType>;
    requires !std::same_as<std::invoke_result_t<FuncType, ItemType>, void>;
};

template <typename IterType>
concept Iterator = requires(IterType i)
{
    i.operator*();
    i.operator++();
    i.operator!=(i);
};

template <typename ArrayType>
concept Array = requires(ArrayType a)
{
    {
        std::begin(a)
        } -> Iterator;
    {
        std::end(a)
        } -> Iterator;
};

template <typename ItemType, typename IncrementType>
concept Incrementable = requires(const ItemType &i, const IncrementType &inc)
{
    {
        i + inc
        } -> std::same_as<ItemType>;
    {
        i == i
        } -> std::same_as<bool>;
};

}

CO_NAMESPACE_END