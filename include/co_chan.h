#pragma once

#include "co_binary_semaphore.h"
#include "co_condition_variable.h"
#include "co_define.h"
#include "co_mutex.h"
#include "co_nocopy.h"
#include <deque>
#include <mutex>
#include <optional>

CO_NAMESPACE_BEGIN

template <typename ValueType, int MaxSize>
class co_chan : public co_nocopy
{
private:
    std::deque<ValueType> data__;
    bool                  closed__ { false };
    mutable co_mutex      mu__;
    co_condition_variable full_cond__;
    co_condition_variable empty_cond__;

    co_binary_semaphore read_sem__ { 0 };
    co_binary_semaphore write_sem__ { 0 };

public:
    class iterator
    {
    private:
        co_chan*  ch__;
        ValueType value__;
        bool      closed__ { true };

    public:
        iterator(co_chan* ch);
        iterator() = default;
        iterator   operator++();
        ValueType& operator*();
        ValueType* operator->();
        bool       operator==(const iterator& other);
    };

    iterator begin();
    iterator end();

    bool                     push(ValueType value);
    std::optional<ValueType> pop();
    void                     close();
    bool                     closed() const;
};

template <typename ValueType, int MaxSize>
bool operator<(co_chan<ValueType, MaxSize>& ch, ValueType value);

template <typename ValueType, int MaxSize>
bool operator>(co_chan<ValueType, MaxSize>& ch, ValueType& value);

template <typename ValueType, int MaxSize>
co_chan<ValueType, MaxSize>& operator<<(co_chan<ValueType, MaxSize>& ch, ValueType value);

template <typename ValueType, int MaxSize>
co_chan<ValueType, MaxSize>& operator>>(co_chan<ValueType, MaxSize>& ch, ValueType& value);

// 模板实现

template <typename ValueType, int MaxSize>
bool operator<(co_chan<ValueType, MaxSize>& ch, ValueType value)
{
    return ch.push(value);
}

template <typename ValueType, int MaxSize>
bool operator>(co_chan<ValueType, MaxSize>& ch, ValueType& value)
{
    auto ret = ch.pop();
    if (!ret)
    {
        return false;
    }
    value = ret.value();
    return true;
}

template <typename ValueType, int MaxSize>
co_chan<ValueType, MaxSize>& operator<<(co_chan<ValueType, MaxSize>& ch, ValueType value)
{
    ch.push(value);
    return ch;
}

template <typename ValueType, int MaxSize>
co_chan<ValueType, MaxSize>& operator>>(co_chan<ValueType, MaxSize>& ch, ValueType& value)
{
    value = ch.pop().value();
    return ch;
}

template <typename ValueType, int MaxSize>
bool co_chan<ValueType, MaxSize>::push(ValueType value)
{
    std::unique_lock<co_mutex> lock(mu__);
    if (closed__)
    {
        // CO_O_DEBUG("closed");
        return false;
    }
    if constexpr (MaxSize > 0) // 当MaxSize < 0，是无限长度的chan
    {
        if (data__.size() == MaxSize)
        {
            full_cond__.wait(lock, [this] { return closed__ || data__.size() < MaxSize; });
            if (closed__)
            {
                // CO_O_DEBUG("closed");
                return false;
            }
        }
    }
    else if constexpr (MaxSize == 0) // 无缓冲chan
    {
        lock.unlock();
        // CO_O_DEBUG("acquire read");
        read_sem__.acquire();
        lock.lock();
    }
    // CO_O_DEBUG("push value");
    data__.push_back(value);
    if constexpr (MaxSize == 0)
    {
        lock.unlock();
        // CO_O_DEBUG("release write");
        write_sem__.release();
        lock.lock();
    }
    else
    {
        empty_cond__.notify_one();
    }
    return true;
}

template <typename ValueType, int MaxSize>
std::optional<ValueType> co_chan<ValueType, MaxSize>::pop()
{
    std::optional<ValueType>   ret;
    std::unique_lock<co_mutex> lock(mu__);

    if constexpr (MaxSize != 0)
    {
        if (data__.empty())
        {
            if (closed__)
            {
                // CO_O_DEBUG("closed");
                return ret;
            }
            empty_cond__.wait(lock, [this] { return closed__ || !data__.empty(); });
            if (data__.empty())
            {
                // CO_O_DEBUG("closed");
                return ret;
            }
        }
    }
    else
    {
        if (closed__)
        {
            return ret;
        }
        lock.unlock();
        // CO_O_DEBUG("release read");
        read_sem__.release();
        // CO_O_DEBUG("acquire write");
        write_sem__.acquire();
        lock.lock();
        if (data__.empty())
        {
            return ret;
        }
    }
    // CO_O_DEBUG("pop value");
    ret = data__.front();
    data__.pop_front();
    if constexpr (MaxSize != 0)
    {
        full_cond__.notify_one();
    }
    return ret;
}

template <typename ValueType, int MaxSize>
void co_chan<ValueType, MaxSize>::close()
{
    std::lock_guard<co_mutex> lock(mu__);
    // CO_O_DEBUG("close chan");
    closed__ = true;
    if constexpr (MaxSize == 0)
    {
        write_sem__.release();
    }
    else
    {
        full_cond__.notify_all();
        empty_cond__.notify_all();
    }
}

template <typename ValueType, int MaxSize>
bool co_chan<ValueType, MaxSize>::closed() const
{
    std::lock_guard<co_mutex> lock(mu__);
    return closed__;
}

// 迭代器相关

template <typename ValueType, int MaxSize>
co_chan<ValueType, MaxSize>::iterator::iterator(co_chan<ValueType, MaxSize>* ch)
    : ch__(ch)
    , closed__(ch->closed__)
{
    ++*this; // 填充第一个元素
}

template <typename ValueType, int MaxSize>
typename co_chan<ValueType, MaxSize>::iterator co_chan<ValueType, MaxSize>::iterator::operator++()
{
    auto curr = ch__->pop();
    if (curr)
    {
        value__ = curr.value();
    }
    else
    {
        closed__ = true;
    }
    return *this;
}

template <typename ValueType, int MaxSize>
ValueType& co_chan<ValueType, MaxSize>::iterator::operator*()
{
    return value__;
}

template <typename ValueType, int MaxSize>
ValueType* co_chan<ValueType, MaxSize>::iterator::operator->()
{
    return &value__;
}

template <typename ValueType, int MaxSize>
bool co_chan<ValueType, MaxSize>::iterator::operator==(const iterator& other)
{
    if (closed__ && other.closed__)
    {
        return true;
    }
    return false;
}

template <typename ValueType, int MaxSize>
typename co_chan<ValueType, MaxSize>::iterator co_chan<ValueType, MaxSize>::begin()
{
    return co_chan<ValueType, MaxSize>::iterator(this);
}

template <typename ValueType, int MaxSize>
typename co_chan<ValueType, MaxSize>::iterator co_chan<ValueType, MaxSize>::end()
{
    return co_chan<ValueType, MaxSize>::iterator();
}

CO_NAMESPACE_END