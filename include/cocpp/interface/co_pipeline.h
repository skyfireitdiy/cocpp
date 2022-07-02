_Pragma("once");
#include "cocpp/comm/co_chan.h"
#include "cocpp/interface/co.h"
#include "cocpp/interface/pipeline/co_pipeline_op.h"
#include "cocpp/utils/co_noncopyable.h"
#include <algorithm>
#include <cstddef>
#include <cstdio>
#include <iterator>
#include <ranges>
#include <type_traits>
#include <vector>

CO_NAMESPACE_BEGIN

// 前置声明
namespace pipeline
{
struct chan;
template <Collection Type>
struct to;
template <typename FilterType>
struct filter;
template <typename ReduceType, typename InitType>
struct reduce;
struct stream;
struct take;
struct skip;
template <typename FuncType>
struct fork;
}

////////////////////////////////////////////////////////////////////////////

template <typename ItemType>
class co_pipeline final : private co_noncopyable
{
private:
    std::shared_ptr<co> co__;
    co_chan<ItemType>   channel__;

    template <typename FilterType>
    requires pipeline::FilterFunc<FilterType, ItemType>
    co_pipeline(co_chan<ItemType> ch, const pipeline::filter<FilterType>& filter);

    template <typename ReduceType, typename InitType>
    requires pipeline::ReduceFunc<ReduceType, InitType, ItemType>
    co_pipeline(co_chan<InitType> ch, const pipeline::reduce<ReduceType, InitType>& reducer);

    co_pipeline(co_chan<ItemType> ch, const pipeline::take& take);

    co_pipeline(co_chan<ItemType> ch, const pipeline::skip& skip);

    template <typename FuncType, typename OldType>
    requires pipeline::ReturnIsNotVoid<FuncType, OldType>
    co_pipeline(co_chan<OldType> ch, const pipeline::fork<FuncType>& fork);

public : template <typename FuncType>
    requires pipeline::PipelineInitFunc<FuncType, ItemType>
    co_pipeline(FuncType init_func, int max_chan_size = -1);

    template <typename CollectionType>
    requires pipeline::Iterable<CollectionType>
    co_pipeline(const CollectionType& col, int max_chan_size = -1);

    template <pipeline::Array ArrayType>
    co_pipeline(const ArrayType& arr, int max_chan_size = -1);

    co_pipeline(const std::initializer_list<ItemType>& col, int max_chan_size = -1);

    template <pipeline::Iterator IterType>
    co_pipeline(IterType begin, IterType end, int max_chan_size = -1);

    template <typename IncrementType>
    requires pipeline::Incrementable<ItemType, IncrementType>
    co_pipeline(const ItemType& begin, const ItemType& end, const IncrementType& inc, int max_chan_size = -1);

    template <typename FuncType>
    requires pipeline::ReturnIsNotVoid<FuncType, ItemType>
    auto operator|(FuncType func);

    auto operator|(const pipeline::chan&);

    template <typename CollectionType>
    requires pipeline::To<ItemType, CollectionType>
    auto operator|(const pipeline::to<CollectionType>&);

    template <typename FilterType>
    requires pipeline::FilterFunc<FilterType, ItemType>
    auto operator|(const pipeline::filter<FilterType>&);

    template <typename ReduceType, typename InitType>
    requires pipeline::ReduceFunc<ReduceType, InitType, ItemType>
    auto operator|(const pipeline::reduce<ReduceType, InitType>&);

    auto operator|(const pipeline::take& take);

    auto operator|(const pipeline::skip& skip);

    template <typename FuncType>
    requires pipeline::ReturnIsNotVoid<FuncType, ItemType>
    auto operator|(const pipeline::fork<FuncType>& fork);
};

////////////////////////////////////////////////////////////////////////////

template <pipeline::Collection CollectionType>
auto operator|(const CollectionType& c, const pipeline::stream& p)
{
    return co_pipeline<std::decay_t<decltype(*std::declval<CollectionType>().begin())>>(c);
}

template <pipeline::Array ArrayType>
auto operator|(const ArrayType& a, const pipeline::stream& p)
{
    return co_pipeline<std::decay_t<decltype(*std::begin(std::declval<ArrayType>()))>>(a);
}

////////////////////////////////////////////////////////////////////////////

template <typename ItemType>
template <typename IncrementType>
requires pipeline::Incrementable<ItemType, IncrementType>
co_pipeline<ItemType>::co_pipeline(const ItemType& begin, const ItemType& end, const IncrementType& inc, int max_chan_size)
    : channel__(max_chan_size)
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
->co_pipeline<std::decay_t<decltype(*std::declval<std::invoke_result_t<FuncType>>())>>;

template <typename CollectionType>
requires pipeline::Iterable<CollectionType>
co_pipeline(const CollectionType&)
->co_pipeline<std::decay_t<decltype(*std::declval<CollectionType>().begin())>>;

template <typename ItemType>
template <pipeline::Iterator IterType>
co_pipeline<ItemType>::co_pipeline(IterType begin, IterType end, int max_chan_size)
    : channel__(max_chan_size)
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

template <typename ItemType>
template <pipeline::Array ArrayType>
co_pipeline<ItemType>::co_pipeline(const ArrayType& arr, int max_chan_size)
    : co_pipeline<ItemType>(std::begin(arr), std::end(arr), max_chan_size)
{
}

template <typename ItemType>
template <typename FuncType>
requires pipeline::PipelineInitFunc<FuncType, ItemType>
co_pipeline<ItemType>::co_pipeline(FuncType init_func, int max_chan_size)
    : channel__(max_chan_size)
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

template <typename ItemType>
template <typename CollectionType>
requires pipeline::To<ItemType, CollectionType>
auto co_pipeline<ItemType>::operator|(const pipeline::to<CollectionType>&)
{
    CollectionType ret;
    for (const auto& t : channel__)
    {
        ret.insert(ret.end(), t);
    }
    return ret;
}

template <typename ItemType>
template <typename CollectionType>
requires pipeline::Iterable<CollectionType>
co_pipeline<ItemType>::co_pipeline(const CollectionType& col, int max_chan_size)
    : channel__(max_chan_size)
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

template <typename ItemType>
co_pipeline<ItemType>::co_pipeline(const std::initializer_list<ItemType>& col, int max_chan_size)
    : channel__(max_chan_size)
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

template <typename ItemType>
template <typename FuncType>
requires pipeline::ReturnIsNotVoid<FuncType, ItemType>
auto co_pipeline<ItemType>::operator|(FuncType func)
{
    return co_pipeline<std::invoke_result_t<FuncType, ItemType>>([ch = channel__, func]() mutable -> std::optional<std::invoke_result_t<FuncType, ItemType>> {
        auto item = ch.pop();
        if (!item)
        {
            return std::nullopt;
        }
        return func(*item);
    });
}

template <typename ItemType>
template <typename FuncType, typename OldType>
requires pipeline::ReturnIsNotVoid<FuncType, OldType>
co_pipeline<ItemType>::co_pipeline(co_chan<OldType> ch, const pipeline::fork<FuncType>& fork)
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

template <typename ItemType>
template <typename FuncType>
requires pipeline::ReturnIsNotVoid<FuncType, ItemType>
auto co_pipeline<ItemType>::operator|(const pipeline::fork<FuncType>& fork)
{
    return co_pipeline<std::invoke_result_t<FuncType, ItemType>>(channel__, fork);
}

template <typename ItemType>
auto co_pipeline<ItemType>::operator|(const pipeline::chan&)
{
    return channel__;
}

template <typename ItemType>
template <typename FilterType>
requires pipeline::FilterFunc<FilterType, ItemType>
co_pipeline<ItemType>::co_pipeline(co_chan<ItemType> ch, const pipeline::filter<FilterType>& filter)
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

template <typename ItemType>
template <typename FilterType>
requires pipeline::FilterFunc<FilterType, ItemType>
auto co_pipeline<ItemType>::operator|(const pipeline::filter<FilterType>& filter)
{
    return co_pipeline<ItemType>(channel__, filter);
}

template <typename ItemType>
template <typename ReduceType, typename InitType>
requires pipeline::ReduceFunc<ReduceType, InitType, ItemType>
auto co_pipeline<ItemType>::operator|(const pipeline::reduce<ReduceType, InitType>& reducer)
{
    return co_pipeline<InitType>(channel__, reducer);
}

template <typename ItemType>
template <typename ReduceType, typename InitType>
requires pipeline::ReduceFunc<ReduceType, InitType, ItemType>
co_pipeline<ItemType>::co_pipeline(co_chan<InitType> ch, const pipeline::reduce<ReduceType, InitType>& reducer)
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

template <typename ItemType>
auto co_pipeline<ItemType>::operator|(const pipeline::take& take)
{
    return co_pipeline<ItemType>(channel__, take);
}

template <typename ItemType>
auto co_pipeline<ItemType>::operator|(const pipeline::skip& skip)
{
    return co_pipeline<ItemType>(channel__, skip);
}

template <typename ItemType>
co_pipeline<ItemType>::co_pipeline(co_chan<ItemType> ch, const pipeline::take& take)
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

template <typename ItemType>
co_pipeline<ItemType>::co_pipeline(co_chan<ItemType> ch, const pipeline::skip& skip)
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