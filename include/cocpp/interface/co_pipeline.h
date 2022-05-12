#include "cocpp/comm/co_chan.h"
#include "cocpp/interface/co.h"
#include "cocpp/utils/co_noncopyable.h"
#include <algorithm>
#include <iterator>
#include <ranges>
#include <type_traits>

CO_NAMESPACE_BEGIN

namespace pipeline
{
struct chan_t
{
};

template <typename Type>
struct to_t
{
};

template <typename FilterType>
struct filter_t
{
    FilterType filter;
    filter_t(FilterType f)
        : filter(f)
    {
    }
};

template <typename ItemType, typename ResultType>
struct reduce_t
{
    std::function<ResultType(const ResultType&, const ItemType&)> reducer__;
    ResultType                                                    init__;
    reduce_t(std::function<ResultType(const ResultType&, const ItemType&)> r, ResultType init)
        : reducer__(r)
        , init__(init)
    {
    }
};

constexpr chan_t
chan()
{
    return chan_t {};
};

template <typename Type>
constexpr to_t<Type> to()
{
    return to_t<Type> {};
}

template <typename FilterType>
filter_t<FilterType> filter(FilterType f)
{
    return filter_t<FilterType>(f);
}

template <typename ItemType, typename ResultType>
reduce_t<ItemType, ResultType> reduce(std::function<ResultType(const ResultType&, const ItemType&)> r, ResultType init)
{
    return reduce_t<ItemType, ResultType>(std::function<ResultType(const ResultType&, const ItemType&)>(r), init);
}

}

template <typename CollectionType>
concept Iterable = requires(CollectionType c)
{
    c.begin();
    c.end();
};

template <typename FuncType, typename ItemType>
concept PipelineInitFuncType = requires(FuncType f, ItemType i)
{
    requires std::invocable<FuncType>;
    requires std::same_as<std::invoke_result_t<FuncType>, std::optional<ItemType>>;
};

template <typename ItemType, typename CollectionType>
concept CanTo = std::is_same_v<std::decay_t<decltype(*std::declval<CollectionType>().begin())>, ItemType>;

template <typename ItemType, int ChanSize = -1>
class co_pipeline final : private co_noncopyable
{
private:
    std::shared_ptr<co>         co__;
    co_chan<ItemType, ChanSize> channel__;

    template <typename FilterType>
    co_pipeline(co_chan<ItemType, ChanSize> ch, const pipeline::filter_t<FilterType>& filter);

    template <typename SrcType>
    co_pipeline(co_chan<SrcType, ChanSize> ch, const pipeline::reduce_t<SrcType, ItemType>& reducer);

public:
    template <typename FuncType>
    requires PipelineInitFuncType<FuncType, ItemType>
    co_pipeline(FuncType init_func);

    template <typename CollectionType>
    requires Iterable<CollectionType>
    co_pipeline(const CollectionType& col);

    co_pipeline(const std::initializer_list<ItemType>& col);

    template <typename FuncType>
    requires std::invocable<FuncType, ItemType>
        co_pipeline<std::invoke_result_t<FuncType, ItemType>, ChanSize>
        operator|(FuncType func);

    co_chan<ItemType, ChanSize> operator|(const pipeline::chan_t&);

    template <typename CollectionType>
    requires CanTo<ItemType, CollectionType>
        CollectionType
        operator|(const pipeline::to_t<CollectionType>&);

    template <typename FilterType>
    co_pipeline<ItemType, ChanSize>
    operator|(const pipeline::filter_t<FilterType>&);

    template <typename ResultType>
    co_pipeline<ResultType, ChanSize>
    operator|(const pipeline::reduce_t<ItemType, ResultType>&);
};

////////////////////////////////////////////////////////////////////////////

template <typename FuncType>
requires std::invocable<FuncType>
co_pipeline(FuncType)
->co_pipeline<std::decay_t<decltype(*std::declval<std::invoke_result_t<FuncType>>())>, -1>;

template <typename CollectionType>
requires Iterable<CollectionType>
co_pipeline(const CollectionType&)
->co_pipeline<std::decay_t<decltype(*std::declval<CollectionType>().begin())>, -1>;

template <typename ItemType, int ChanSize>
template <typename FuncType>
requires PipelineInitFuncType<FuncType, ItemType>
co_pipeline<ItemType, ChanSize>::co_pipeline(FuncType init_func)
{
    co__ = std::make_shared<co>([ch = channel__, init_func]() mutable {
        while (true)
        {
            auto item = init_func();
            if (!item)
            {
                ch.close();
                break;
            }
            ch.push(*item);
        }
    });
    co__->detach();
}

template <typename ItemType, int ChanSize>
template <typename CollectionType>
requires CanTo<ItemType, CollectionType>
    CollectionType
    co_pipeline<ItemType, ChanSize>::operator|(const pipeline::to_t<CollectionType>&)
{
    CollectionType ret;
    for (const auto& t : channel__)
    {
        ret.insert(ret.end(), t);
    }
    return ret;
}

template <typename ItemType, int ChanSize>
template <typename CollectionType>
requires Iterable<CollectionType>
co_pipeline<ItemType, ChanSize>::co_pipeline(const CollectionType& col)
{
    co__ = std::make_shared<co>([ch = channel__, col]() mutable {
        std::ranges::for_each(col, [ch](auto& item) mutable {
            ch.push(item);
        });
        ch.close();
    });
    co__->detach();
}

template <typename ItemType, int ChanSize>
co_pipeline<ItemType, ChanSize>::co_pipeline(const std::initializer_list<ItemType>& col)
{
    co__ = std::make_shared<co>([ch = channel__, col]() mutable {
        std::ranges::for_each(col, [ch](auto& item) mutable {
            ch.push(item);
        });
        ch.close();
    });
    co__->detach();
}

template <typename ItemType, int ChanSize>
template <typename FuncType>
requires std::invocable<FuncType, ItemType>
    co_pipeline<std::invoke_result_t<FuncType, ItemType>, ChanSize>
    co_pipeline<ItemType, ChanSize>::operator|(FuncType func)
{
    return co_pipeline<std::invoke_result_t<FuncType, ItemType>, ChanSize>([ch = channel__, func]() mutable -> std::optional<std::invoke_result_t<FuncType, ItemType>> {
        auto item = ch.pop();
        if (!item)
        {
            return std::nullopt;
        }
        return func(*item);
    });
}

template <typename ItemType, int ChanSize>
co_chan<ItemType, ChanSize> co_pipeline<ItemType, ChanSize>::operator|(const pipeline::chan_t&)
{
    return channel__;
}

template <typename ItemType, int ChanSize>
template <typename FilterType>
co_pipeline<ItemType, ChanSize>::co_pipeline(co_chan<ItemType, ChanSize> ch, const pipeline::filter_t<FilterType>& filter)
{
    co__ = std::make_shared<co>([this_ch = channel__, ch, filter]() mutable {
        for (const auto& item : ch)
        {
            if (filter.filter(item))
            {
                this_ch.push(item);
            }
        }

        this_ch.close();
    });
    co__->detach();
}

template <typename ItemType, int ChanSize>
template <typename FilterType>
co_pipeline<ItemType, ChanSize>
co_pipeline<ItemType, ChanSize>::operator|(const pipeline::filter_t<FilterType>& filter)
{
    return co_pipeline<ItemType, ChanSize>(channel__, filter);
}

template <typename ItemType, int ChanSize>
template <typename ResultType>
co_pipeline<ResultType, ChanSize>
co_pipeline<ItemType, ChanSize>::operator|(const pipeline::reduce_t<ItemType, ResultType>& reducer)
{
    return co_pipeline<ResultType, ChanSize>(channel__, reducer);
}

template <typename ItemType, int ChanSize>
template <typename SrcType>
co_pipeline<ItemType, ChanSize>::co_pipeline(co_chan<SrcType, ChanSize> ch, const pipeline::reduce_t<SrcType, ItemType>& reducer)
{
    co__ = std::make_shared<co>([this_ch = channel__, ch, reducer]() mutable {
        auto result = reducer.init__;
        for (const auto& item : ch)
        {
            result = reducer.reducer__(result, item);
        }
        this_ch.push(result);
        this_ch.close();
    });
    co__->detach();
}

CO_NAMESPACE_END