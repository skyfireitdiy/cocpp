#include "cocpp/comm/co_chan.h"
#include "cocpp/interface/co.h"
#include "cocpp/utils/co_noncopyable.h"
#include <type_traits>

CO_NAMESPACE_BEGIN

template <typename ItemType, int ChanSize = -1>
class co_pipeline final : private co_noncopyable
{
private:
    std::shared_ptr<co>         co__;
    co_chan<ItemType, ChanSize> channel__;

public:
    co_pipeline(std::function<std::optional<ItemType>()> init_func);

    template <typename FuncType>
    // requires std::common_with<FuncType, std::function<RetType(const ItemType&)>>
    co_pipeline<std::invoke_result_t<FuncType, ItemType>, ChanSize>
    operator|(FuncType func);

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
template <typename FuncType>
// requires std::common_with<FuncType, std::function<RetType(const ItemType&)>>
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
co_chan<ItemType, ChanSize> co_pipeline<ItemType, ChanSize>::chan()
{
    return channel__;
}

CO_NAMESPACE_END