#include "cocpp/comm/co_chan.h"
#include "cocpp/interface/co.h"
#include "cocpp/utils/co_noncopyable.h"
#include <algorithm>
#include <iterator>
#include <ranges>
#include <type_traits>

CO_NAMESPACE_BEGIN

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
    requires std::same_as<std::invoke_result_t<FuncType>, std::optional<ItemType>>;
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
    requires std::same_as < std::invoke_result_t<FilterType, ItemType>,
    bool > ;
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

///////////////////////////////////////////////////////////////////////
namespace pipeline
{
    struct chan_t
    {
    };

    template <Collection Type>
    struct to_t
    {
    };

    template <typename FilterType>
    struct filter_t
    {
        FilterType filter__;
        filter_t(FilterType f)
            : filter__(f)
        {
        }
    };

    template <typename ReduceType, typename InitType>
    struct reduce_t
    {
        ReduceType reducer__;
        InitType   init__;
        reduce_t(ReduceType reducer, InitType init)
            : reducer__(reducer)
            , init__(init)
        {
        }
    };

    template <int Size>
    struct stream_t
    {
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

    template <typename ReduceType, typename InitType>
    reduce_t<ReduceType, InitType> reduce(ReduceType reducer, InitType init)
    {
        return reduce_t<ReduceType, InitType>(reducer, init);
    }

    template <int Size>
    constexpr stream_t<Size> stream() { return stream_t<Size> {}; }

}

////////////////////////////////////////////////////////////////////////////

template <typename ItemType, int ChanSize = -1>
class co_pipeline final : private co_noncopyable
{
private:
    std::shared_ptr<co>         co__;
    co_chan<ItemType, ChanSize> channel__;

    template <typename FilterType>
    requires FilterFunc<FilterType, ItemType>
    co_pipeline(co_chan<ItemType, ChanSize> ch, const pipeline::filter_t<FilterType>& filter);

    template <typename ReduceType, typename InitType>
    requires ReduceFunc<ReduceType, InitType, ItemType>
    co_pipeline(co_chan<InitType, ChanSize> ch, const pipeline::reduce_t<ReduceType, InitType>& reducer);

public : template <typename FuncType>
    requires PipelineInitFunc<FuncType, ItemType>
    co_pipeline(FuncType init_func);

    template <typename CollectionType>
    requires Iterable<CollectionType>
    co_pipeline(const CollectionType& col);

    template <Array ArrayType>
    co_pipeline(const ArrayType& arr);

    co_pipeline(const std::initializer_list<ItemType>& col);

    template <Iterator IterType>
    co_pipeline(IterType begin, IterType end);

    template <typename FuncType>
    requires ReturnIsNotVoid<FuncType, ItemType>
        co_pipeline<std::invoke_result_t<FuncType, ItemType>, ChanSize>
        operator|(FuncType func);

    co_chan<ItemType, ChanSize> operator|(const pipeline::chan_t&);

    template <typename CollectionType>
    requires To<ItemType, CollectionType>
        CollectionType
        operator|(const pipeline::to_t<CollectionType>&);

    template <typename FilterType>
    requires FilterFunc<FilterType, ItemType>
        co_pipeline<ItemType, ChanSize>
        operator|(const pipeline::filter_t<FilterType>&);

    template <typename ReduceType, typename InitType>
    requires ReduceFunc<ReduceType, InitType, ItemType>
        co_pipeline<InitType, ChanSize>
        operator|(const pipeline::reduce_t<ReduceType, InitType>&);
};

////////////////////////////////////////////////////////////////////////////

template <Collection CollectionType, int Size>
co_pipeline<std::decay_t<decltype(*std::declval<CollectionType>().begin())>, Size> operator|(const CollectionType& c, const pipeline::stream_t<Size>& p)
{
    return co_pipeline<std::decay_t<decltype(*std::declval<CollectionType>().begin())>, Size>(c);
}

template <Array ArrayType, int Size>
co_pipeline<std::decay_t<decltype(*std::begin(std::declval<ArrayType>()))>, Size> operator|(const ArrayType& a, const pipeline::stream_t<Size>& p)
{
    return co_pipeline<std::decay_t<decltype(*std::begin(std::declval<ArrayType>()))>, Size>(a);
}

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
template <Iterator IterType>
co_pipeline<ItemType, ChanSize>::co_pipeline(IterType begin, IterType end)
{
    co__ = std::make_shared<co>([ch = channel__, begin, end]() mutable {
        std::for_each(begin, end, [ch](auto& item) mutable {
            ch.push(item);
        });
        ch.close();
    });
    co__->detach();
}

template <typename ItemType, int ChanSize>
template <Array ArrayType>
co_pipeline<ItemType, ChanSize>::co_pipeline(const ArrayType& arr)
    : co_pipeline<ItemType, ChanSize>(std::begin(arr), std::end(arr))
{
}

template <typename ItemType, int ChanSize>
template <typename FuncType>
requires PipelineInitFunc<FuncType, ItemType>
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
requires To<ItemType, CollectionType>
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
requires ReturnIsNotVoid<FuncType, ItemType>
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
requires FilterFunc<FilterType, ItemType>
co_pipeline<ItemType, ChanSize>::co_pipeline(co_chan<ItemType, ChanSize> ch, const pipeline::filter_t<FilterType>& filter)
{
    co__ = std::make_shared<co>([this_ch = channel__, ch, filter]() mutable {
        for (const auto& item : ch)
        {
            if (filter.filter__(item))
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
requires FilterFunc<FilterType, ItemType>
    co_pipeline<ItemType, ChanSize>
    co_pipeline<ItemType, ChanSize>::operator|(const pipeline::filter_t<FilterType>& filter)
{
    return co_pipeline<ItemType, ChanSize>(channel__, filter);
}

template <typename ItemType, int ChanSize>
template <typename ReduceType, typename InitType>
requires ReduceFunc<ReduceType, InitType, ItemType>
    co_pipeline<InitType, ChanSize>
    co_pipeline<ItemType, ChanSize>::operator|(const pipeline::reduce_t<ReduceType, InitType>& reducer)
{
    return co_pipeline<InitType, ChanSize>(channel__, reducer);
}

template <typename ItemType, int ChanSize>
template <typename ReduceType, typename InitType>
requires ReduceFunc<ReduceType, InitType, ItemType>
co_pipeline<ItemType, ChanSize>::co_pipeline(co_chan<InitType, ChanSize> ch, const pipeline::reduce_t<ReduceType, InitType>& reducer)
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