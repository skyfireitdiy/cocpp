#include "cocpp/comm/co_event.h"
#include "cocpp/core/co_ctx.h"
#include <algorithm>
#include <cstddef>
_Pragma("once");

#include "cocpp/core/co_define.h"
#include "cocpp/sync/co_condition_variable.h"
#include "cocpp/sync/co_mutex.h"
#include <concepts>
#include <deque>
#include <mutex>
#include <optional>

CO_NAMESPACE_BEGIN

namespace chan
{

};

template <std::copyable ValueType>
class co_chan final
{
private:
    std::shared_ptr<std::deque<ValueType>> data__ { std::make_shared<std::deque<ValueType>>() };
    std::shared_ptr<bool>                  closed__ { std::make_shared<bool>(false) };
    mutable std::shared_ptr<co_mutex>      mu__ { std::make_shared<co_mutex>() };
    std::shared_ptr<co_condition_variable> cv_full__  = { std::make_shared<co_condition_variable>() };
    std::shared_ptr<co_condition_variable> cv_empty__ = { std::make_shared<co_condition_variable>() };
    const int                              max_size__ { -1 };

public:
    class iterator
    {
    private:
        co_chan*                 ch__ { nullptr };
        std::optional<ValueType> value__;

    public:
        iterator(co_chan* ch);
        iterator() = default;
        iterator   operator++();
        ValueType& operator*();
        ValueType* operator->();
        bool       operator==(const iterator& other);
        // bool       operator!=(const iterator& other);
    };

    iterator begin();
    iterator end();

    bool                     push(ValueType value);
    std::optional<ValueType> pop();
    void                     close();
    bool                     closed() const;
    bool                     empty() const;
    int                      max_size() const;

    co_chan(int max_size = -1);
};

template <std::copyable ValueType>
co_chan<ValueType>::co_chan(int max_size)
    : max_size__(max_size)
{
}

template <std::copyable ValueType>
bool operator<(co_chan<ValueType>& ch, ValueType value);

template <std::copyable ValueType>
bool operator>(co_chan<ValueType>& ch, ValueType& value);

template <std::copyable ValueType>
co_chan<ValueType>& operator<<(co_chan<ValueType>& ch, ValueType value);

template <std::copyable ValueType>
co_chan<ValueType>& operator>>(co_chan<ValueType>& ch, ValueType& value);

template <std::copyable ValueType>
bool operator<(co_chan<ValueType>& ch, ValueType value)
{
    return ch.push(value);
}

template <std::copyable ValueType>
bool operator>(co_chan<ValueType>& ch, ValueType& value)
{
    auto ret = ch.pop();
    if (!ret)
    {
        return false;
    }
    value = ret.value();
    return true;
}

template <std::copyable ValueType>
co_chan<ValueType>& operator<<(co_chan<ValueType>& ch, ValueType value)
{
    ch.push(value);
    return ch;
}

template <std::copyable ValueType>
co_chan<ValueType>& operator>>(co_chan<ValueType>& ch, ValueType& value)
{
    value = ch.pop().value();
    return ch;
}

template <std::copyable ValueType>
bool co_chan<ValueType>::push(ValueType value)
{
    std::unique_lock lock(*mu__);
    if (*closed__)
    {
        return false;
    }
    auto max_size = max_size__ == 0 ? 1 : max_size__;
    if (max_size > 0)
    {
        if (data__->size() == static_cast<size_t>(max_size))
        {
            cv_full__->wait(lock, [this] { return *closed__ || data__->size() < static_cast<size_t>(max_size__); });
            if (*closed__)
            {
                return false;
            }
        }
    }

    data__->push_back(value);
    cv_empty__->notify_one();

    if (max_size__ == 0)
    {
        cv_full__->wait(lock, [this] { return *closed__ || data__->empty(); });
    }

    return true;
}

template <std::copyable ValueType>
std::optional<ValueType> co_chan<ValueType>::pop()
{
    std::optional<ValueType> ret;
    std::unique_lock         lock(*mu__);

    if (data__->empty())
    {
        if (*closed__)
        {
            return ret;
        }
        cv_empty__->wait(lock, [this] {
            return *closed__ || !data__->empty();
        });
        if (data__->empty())
        {
            return ret;
        }
    }

    ret = data__->front();
    data__->pop_front();
    cv_full__->notify_one();
    return ret;
}

template <std::copyable ValueType>
void co_chan<ValueType>::close()
{
    std::scoped_lock lock(*mu__);
    *closed__ = true;
    cv_full__->notify_all();
    cv_empty__->notify_all();
}

template <std::copyable ValueType>
int co_chan<ValueType>::max_size() const { return max_size__; }

template <std::copyable ValueType>
bool co_chan<ValueType>::closed() const
{
    std::scoped_lock lock(*mu__);
    return *closed__;
}

template <std::copyable ValueType>
bool co_chan<ValueType>::empty() const
{
    std::scoped_lock lock(*mu__);
    return data__->empty();
}

template <std::copyable ValueType>
co_chan<ValueType>::iterator::iterator(co_chan<ValueType>* ch)
    : ch__(ch)
{
    ++*this;
}

template <std::copyable ValueType>
typename co_chan<ValueType>::iterator co_chan<ValueType>::iterator::operator++()
{
    value__ = ch__->pop();
    return *this;
}

template <std::copyable ValueType>
ValueType& co_chan<ValueType>::iterator::operator*()
{
    return *value__;
}

template <std::copyable ValueType>
ValueType* co_chan<ValueType>::iterator::operator->()
{
    return &*value__;
}

template <std::copyable ValueType>
bool co_chan<ValueType>::iterator::operator==(const iterator& other)
{
    if (ch__ == other.ch__)
    {
        return true;
    }
    if (!ch__ || (ch__->closed() && ch__->empty() && !value__))
    {
        if (!other.ch__ || (other.ch__->closed() && other.ch__->empty() && !other.value__))
        {
            return true;
        }
    }
    return false;
}

// template <std::copyable ValueType>
// bool co_chan<ValueType>::iterator::operator!=(const iterator& other)
// {
//     return !(*this == other);
// }

template <std::copyable ValueType>
typename co_chan<ValueType>::iterator co_chan<ValueType>::begin()
{
    return co_chan<ValueType>::iterator(this);
}

template <std::copyable ValueType>
typename co_chan<ValueType>::iterator co_chan<ValueType>::end()
{
    return co_chan<ValueType>::iterator();
}

CO_NAMESPACE_END