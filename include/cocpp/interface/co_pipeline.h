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

template <typename T>
struct to_t
{
};

template <typename T>
struct filter_t
{
    std::function<bool(T)> filter;
    filter_t(std::function<bool(T)> f)
        : filter(f)
    {
    }
};

constexpr chan_t
chan()
{
    return chan_t {};
};

template <typename T>
constexpr to_t<T> to()
{
    return to_t<T> {};
}

template <typename T>
filter_t<T> filter(std::function<bool(T)> f)
{
    return filter_t<T>(std::function<bool(T)>(f));
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

    co_pipeline(co_chan<ItemType, ChanSize> ch, std::function<bool(const ItemType&)> filter);

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

    co_pipeline<ItemType, ChanSize>
    operator|(const pipeline::filter_t<ItemType>&);
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
co_pipeline<ItemType, ChanSize>::co_pipeline(co_chan<ItemType, ChanSize> ch, std::function<bool(const ItemType&)> filter)
{
    co__ = std::make_shared<co>([this_ch = channel__, ch, filter]() mutable {
        for (const auto& item : ch)
        {
            if (filter(item))
            {
                this_ch.push(item);
            }
        }

        this_ch.close();
    });
    co__->detach();
}

template <typename ItemType, int ChanSize>
co_pipeline<ItemType, ChanSize>
co_pipeline<ItemType, ChanSize>::operator|(const pipeline::filter_t<ItemType>& filter)
{
    return co_pipeline<ItemType, ChanSize>(channel__, filter.filter);
}

CO_NAMESPACE_END