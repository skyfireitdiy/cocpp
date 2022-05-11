#include "cocpp/comm/co_chan.h"
#include "cocpp/interface/co.h"
#include "cocpp/utils/co_noncopyable.h"

CO_NAMESPACE_BEGIN

template <typename ItemType, int ChanSize = -1>
class co_pipeline final : private co_noncopyable
{
private:
    std::shared_ptr<co>         co__;
    co_chan<ItemType, ChanSize> channel__;

public:
    co_pipeline(std::function<std::optional<ItemType>()> init_func);

    template <typename RetType>
    co_pipeline<RetType, ChanSize> operator|(std::function<RetType(const ItemType&)> func);

    co_chan<ItemType, ChanSize> chan();
};

////////////////////////////////////////////////////////////////////////////

template <typename ItemType, int ChanSize>
co_pipeline<ItemType, ChanSize>::co_pipeline(std::function<std::optional<ItemType>()> init_func)
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
template <typename RetType>
co_pipeline<RetType, ChanSize> co_pipeline<ItemType, ChanSize>::operator|(std::function<RetType(const ItemType&)> func)
{
    return co_pipeline<RetType, ChanSize>([ch = channel__, func]() mutable -> std::optional<RetType> {
        auto item = ch.pop();
        if (!item)
        {
            return std::nullopt;
        }
        return func(*item);
    });
}

template <typename ItemType, int ChanSize>
co_chan<ItemType, ChanSize> co_pipeline<ItemType, ChanSize>::chan()
{
    return channel__;
}

CO_NAMESPACE_END