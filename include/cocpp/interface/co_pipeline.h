#include "cocpp/comm/co_chan.h"
#include "cocpp/interface/co.h"
#include "cocpp/utils/co_noncopyable.h"
#include <algorithm>
#include <cstdio>
#include <iterator>
#include <ranges>
#include <type_traits>
#include <vector>

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

template <typename ItemType, typename IncrementType>
concept Incrementable = requires(const ItemType& i, const IncrementType& inc)
{
    {
        i + inc
        } -> std::same_as<ItemType>;
    {
        i == i
        } -> std::same_as<bool>;
};

}

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
};

template <typename ReduceType, typename InitType>
struct reduce_t
{
    ReduceType reducer__;
    InitType   init__;
};

template <int Size>
struct stream_t
{
};

struct take_t
{
    size_t take__;
};

struct skip_t
{
    size_t skip__;
};

template <typename FuncType>
struct fork_t
{
    size_t   fork_count__;
    FuncType func__;
};

///////////////////////////////////////////////////////////////////////

constexpr auto chan()
{
    return chan_t {};
};

template <typename Type>
constexpr auto to()
{
    return to_t<Type> {};
}

template <typename FilterType>
auto filter(FilterType f)
{
    return filter_t<FilterType> { .filter__ = f };
}

template <typename ReduceType, typename InitType>
auto reduce(ReduceType reducer, InitType init)
{
    return reduce_t<ReduceType, InitType> { .reducer__ = reducer, .init__ = init };
}

template <int Size>
constexpr auto stream() { return stream_t<Size> {}; }

take_t take(size_t n);

skip_t skip(size_t n);

template <typename FuncType>
auto fork(size_t n, FuncType f)
{
    if (n == 0)
    {
        n = 1;
    }
    return fork_t<FuncType> { .fork_count__ = n, .func__ = f };
}

}

////////////////////////////////////////////////////////////////////////////

template <typename ItemType, int ChanSize = -1>
class co_pipeline final : private co_noncopyable
{
private:
    std::shared_ptr<co>         co__;
    co_chan<ItemType, ChanSize> channel__;

    template <typename FilterType>
    requires pipeline::FilterFunc<FilterType, ItemType>
    co_pipeline(co_chan<ItemType, ChanSize> ch, const pipeline::filter_t<FilterType>& filter);

    template <typename ReduceType, typename InitType>
    requires pipeline::ReduceFunc<ReduceType, InitType, ItemType>
    co_pipeline(co_chan<InitType, ChanSize> ch, const pipeline::reduce_t<ReduceType, InitType>& reducer);

    co_pipeline(co_chan<ItemType, ChanSize> ch, const pipeline::take_t& take);

    co_pipeline(co_chan<ItemType, ChanSize> ch, const pipeline::skip_t& skip);

    template <typename FuncType, typename OldType>
    requires pipeline::ReturnIsNotVoid<FuncType, OldType>
    co_pipeline(co_chan<OldType, ChanSize> ch, const pipeline::fork_t<FuncType>& fork);

public : template <typename FuncType>
    requires pipeline::PipelineInitFunc<FuncType, ItemType>
    co_pipeline(FuncType init_func);

    template <typename CollectionType>
    requires pipeline::Iterable<CollectionType>
    co_pipeline(const CollectionType& col);

    template <pipeline::Array ArrayType>
    co_pipeline(const ArrayType& arr);

    co_pipeline(const std::initializer_list<ItemType>& col);

    template <pipeline::Iterator IterType>
    co_pipeline(IterType begin, IterType end);

    template <typename IncrementType>
    requires pipeline::Incrementable<ItemType, IncrementType>
    co_pipeline(const ItemType& begin, const ItemType& end, const IncrementType& inc);

    template <typename FuncType>
    requires pipeline::ReturnIsNotVoid<FuncType, ItemType>
    auto operator|(FuncType func);

    auto operator|(const pipeline::chan_t&);

    template <typename CollectionType>
    requires pipeline::To<ItemType, CollectionType>
    auto operator|(const pipeline::to_t<CollectionType>&);

    template <typename FilterType>
    requires pipeline::FilterFunc<FilterType, ItemType>
    auto operator|(const pipeline::filter_t<FilterType>&);

    template <typename ReduceType, typename InitType>
    requires pipeline::ReduceFunc<ReduceType, InitType, ItemType>
    auto operator|(const pipeline::reduce_t<ReduceType, InitType>&);

    auto operator|(const pipeline::take_t& take);

    auto operator|(const pipeline::skip_t& skip);

    template <typename FuncType>
    requires pipeline::ReturnIsNotVoid<FuncType, ItemType>
    auto operator|(const pipeline::fork_t<FuncType>& fork);
};

////////////////////////////////////////////////////////////////////////////

template <pipeline::Collection CollectionType, int Size>
auto operator|(const CollectionType& c, const pipeline::stream_t<Size>& p)
{
    return co_pipeline<std::decay_t<decltype(*std::declval<CollectionType>().begin())>, Size>(c);
}

template <pipeline::Array ArrayType, int Size>
auto operator|(const ArrayType& a, const pipeline::stream_t<Size>& p)
{
    return co_pipeline<std::decay_t<decltype(*std::begin(std::declval<ArrayType>()))>, Size>(a);
}

////////////////////////////////////////////////////////////////////////////

template <typename ItemType, int ChanSize>
template <typename IncrementType>
requires pipeline::Incrementable<ItemType, IncrementType>
co_pipeline<ItemType, ChanSize>::co_pipeline(const ItemType& begin, const ItemType& end, const IncrementType& inc)
{
    co__ = std::make_shared<co>([ch = channel__, begin, end, inc]() mutable {
        for (auto i = begin; i != end; i += inc)
        {
            if (!ch.push(i))
            {
                break;
            }
        }
        ch.close();
    });
    co__->detach();
}

template <typename FuncType>
requires std::invocable<FuncType>
co_pipeline(FuncType)
->co_pipeline<std::decay_t<decltype(*std::declval<std::invoke_result_t<FuncType>>())>, -1>;

template <typename CollectionType>
requires pipeline::Iterable<CollectionType>
co_pipeline(const CollectionType&)
->co_pipeline<std::decay_t<decltype(*std::declval<CollectionType>().begin())>, -1>;

template <typename ItemType, int ChanSize>
template <pipeline::Iterator IterType>
co_pipeline<ItemType, ChanSize>::co_pipeline(IterType begin, IterType end)
{
    co__ = std::make_shared<co>([ch = channel__, begin, end]() mutable {
        for (auto i = begin; i != end; ++i)
        {
            if (!ch.push(*i))
            {
                break;
            }
        }
        ch.close();
    });
    co__->detach();
}

template <typename ItemType, int ChanSize>
template <pipeline::Array ArrayType>
co_pipeline<ItemType, ChanSize>::co_pipeline(const ArrayType& arr)
    : co_pipeline<ItemType, ChanSize>(std::begin(arr), std::end(arr))
{
}

template <typename ItemType, int ChanSize>
template <typename FuncType>
requires pipeline::PipelineInitFunc<FuncType, ItemType>
co_pipeline<ItemType, ChanSize>::co_pipeline(FuncType init_func)
{
    co__ = std::make_shared<co>([ch = channel__, init_func]() mutable {
        for (;;)
        {
            auto item = init_func();
            if (!item)
            {
                break;
            }
            if (!ch.push(*item))
            {
                break;
            }
        }
        ch.close();
    });
    co__->detach();
}

template <typename ItemType, int ChanSize>
template <typename CollectionType>
requires pipeline::To<ItemType, CollectionType>
auto co_pipeline<ItemType, ChanSize>::operator|(const pipeline::to_t<CollectionType>&)
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
requires pipeline::Iterable<CollectionType>
co_pipeline<ItemType, ChanSize>::co_pipeline(const CollectionType& col)
{
    co__ = std::make_shared<co>([ch = channel__, col]() mutable {
        for (const auto& t : col)
        {
            if (!ch.push(t))
            {
                break;
            }
        }
        ch.close();
    });
    co__->detach();
}

template <typename ItemType, int ChanSize>
co_pipeline<ItemType, ChanSize>::co_pipeline(const std::initializer_list<ItemType>& col)
{
    co__ = std::make_shared<co>([ch = channel__, col]() mutable {
        for (const auto& t : col)
        {
            if (!ch.push(t))
            {
                break;
            }
        }
        ch.close();
    });
    co__->detach();
}

template <typename ItemType, int ChanSize>
template <typename FuncType>
requires pipeline::ReturnIsNotVoid<FuncType, ItemType>
auto co_pipeline<ItemType, ChanSize>::operator|(FuncType func)
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
template <typename FuncType, typename OldType>
requires pipeline::ReturnIsNotVoid<FuncType, OldType>
co_pipeline<ItemType, ChanSize>::co_pipeline(co_chan<OldType, ChanSize> ch, const pipeline::fork_t<FuncType>& fork)
{
    co__ = std::make_shared<co>([this_ch = channel__, ch, fork]() mutable {
        std::vector<std::shared_ptr<co>> co_list(fork.fork_count__);
        for (auto&& c : co_list)
        {
            c = std::make_shared<co>([this_ch, ch, fork]() mutable {
                while (true)
                {
                    auto item = ch.pop();
                    if (!item)
                    {
                        break;
                    }
                    if (!this_ch.push(fork.func__(*item)))
                    {
                        break;
                    }
                }
            });
        }
        for (auto&& c : co_list)
        {
            c->join();
        }
        this_ch.close();
    });
    co__->detach();
}

template <typename ItemType, int ChanSize>
template <typename FuncType>
requires pipeline::ReturnIsNotVoid<FuncType, ItemType>
auto co_pipeline<ItemType, ChanSize>::operator|(const pipeline::fork_t<FuncType>& fork)
{
    return co_pipeline<std::invoke_result_t<FuncType, ItemType>, ChanSize>(channel__, fork);
}

template <typename ItemType, int ChanSize>
auto co_pipeline<ItemType, ChanSize>::operator|(const pipeline::chan_t&)
{
    return channel__;
}

template <typename ItemType, int ChanSize>
template <typename FilterType>
requires pipeline::FilterFunc<FilterType, ItemType>
co_pipeline<ItemType, ChanSize>::co_pipeline(co_chan<ItemType, ChanSize> ch, const pipeline::filter_t<FilterType>& filter)
{
    co__ = std::make_shared<co>([this_ch = channel__, ch, filter]() mutable {
        for (const auto& item : ch)
        {
            if (filter.filter__(item))
            {
                if (!this_ch.push(item))
                {
                    break;
                }
            }
        }

        this_ch.close();
    });
    co__->detach();
}

template <typename ItemType, int ChanSize>
template <typename FilterType>
requires pipeline::FilterFunc<FilterType, ItemType>
auto co_pipeline<ItemType, ChanSize>::operator|(const pipeline::filter_t<FilterType>& filter)
{
    return co_pipeline<ItemType, ChanSize>(channel__, filter);
}

template <typename ItemType, int ChanSize>
template <typename ReduceType, typename InitType>
requires pipeline::ReduceFunc<ReduceType, InitType, ItemType>
auto co_pipeline<ItemType, ChanSize>::operator|(const pipeline::reduce_t<ReduceType, InitType>& reducer)
{
    return co_pipeline<InitType, ChanSize>(channel__, reducer);
}

template <typename ItemType, int ChanSize>
template <typename ReduceType, typename InitType>
requires pipeline::ReduceFunc<ReduceType, InitType, ItemType>
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

template <typename ItemType, int ChanSize>
auto co_pipeline<ItemType, ChanSize>::operator|(const pipeline::take_t& take)
{
    return co_pipeline<ItemType, ChanSize>(channel__, take);
}

template <typename ItemType, int ChanSize>
auto co_pipeline<ItemType, ChanSize>::operator|(const pipeline::skip_t& skip)
{
    return co_pipeline<ItemType, ChanSize>(channel__, skip);
}

template <typename ItemType, int ChanSize>
co_pipeline<ItemType, ChanSize>::co_pipeline(co_chan<ItemType, ChanSize> ch, const pipeline::take_t& take)
{
    co__ = std::make_shared<co>([this_ch = channel__, ch, take]() mutable {
        for (size_t index = 0; index < take.take__; ++index)
        {
            auto result = ch.pop();
            if (result)
            {
                if (!this_ch.push(*result))
                {
                    break;
                }
            }
            else
            {
                break;
            }
        }

        this_ch.close();
        ch.close();
    });
    co__->detach();
}

template <typename ItemType, int ChanSize>
co_pipeline<ItemType, ChanSize>::co_pipeline(co_chan<ItemType, ChanSize> ch, const pipeline::skip_t& skip)
{
    co__ = std::make_shared<co>([this_ch = channel__, ch, skip]() mutable {
        for (size_t index = 0; index < skip.skip__; ++index)
        {
            auto result = ch.pop();
            if (!result)
            {
                break;
            }
        }
        for (auto&& p : ch)
        {
            if (!this_ch.push(p))
            {
                break;
            }
        }
        this_ch.close();
    });
    co__->detach();
}

CO_NAMESPACE_END