#include "cocpp/mem/co_mem_pool.h"
#include "cocpp/core/co_define.h"
#include "cocpp/core/co_type.h"
#include <bitset>
#include <cassert>
#include <cmath>
#include <cstddef>
#include <new>

CO_NAMESPACE_BEGIN

co_mem_pool::co_mem_pool(size_t min_zone, size_t zone_count, size_t max_cap)
    : min_zone__(min_zone)
    , zone_count__(zone_count)
    , max_cap__(max_cap)
    , mem_pool__(zone_count)
{
}

unsigned long long co_mem_pool::align_2_zone_edge__(unsigned long long size)
{
    std::bitset<sizeof(size) * 8> bits(size);
    int                           first_one = -1;
    int                           last_one  = 0;
    for (size_t i = 0; i < sizeof(size) * 8; ++i)
    {
        if (bits.test(i))
        {
            first_one = first_one == -1 ? i : first_one;
            last_one  = i;
        }
    }
    if (last_one == first_one)
    {
        return last_one;
    }
    return last_one + 1;
}

unsigned long long co_mem_pool::align_size__(unsigned long long size)
{
    return (size + sizeof(void*) - 1) / sizeof(void*) * sizeof(void*);
}

size_t co_mem_pool::get_zone_index__(size_t size) const
{
    unsigned long long zone_index = align_2_zone_edge__(size);
    if (zone_index < min_zone__)
    {
        zone_index = min_zone__;
    }
    return zone_index - min_zone__;
}

co_byte* co_mem_pool::alloc_mem(size_t size)
{
    size_t zone_index = get_zone_index__(size);
    if (zone_index >= zone_count__)
    {
        return reinterpret_cast<co_byte*>(std::aligned_alloc(sizeof(void*), align_size__(size)));
    }

    std::scoped_lock lock(mu__);
    if (mem_pool__[zone_index].empty())
    {
        return reinterpret_cast<co_byte*>(std::aligned_alloc(sizeof(void*), 1 << (zone_index + min_zone__)));
    }
    auto ret = mem_pool__[zone_index].front();
    mem_pool__[zone_index].pop_front();
    return ret;
}

void co_mem_pool::free_mem(co_byte* ptr, size_t size)
{
    size_t zone_index = get_zone_index__(size);
    if (zone_index >= zone_count__)
    {
        std::free(ptr);
        return;
    }
    std::scoped_lock lock(mu__);
    if (mem_pool__[zone_index].size() > max_cap__)
    {
        std::free(ptr);
    }
    else
    {
        mem_pool__[zone_index].push_front(ptr);
    }
}

void co_mem_pool::free_pool()
{
    std::scoped_lock lock(mu__);
    for (auto& zone : mem_pool__)
    {
        for (auto& mem : zone)
        {
            std::free(mem);
        }
        zone.clear();
    }
}

co_mem_pool::~co_mem_pool()
{
    free_pool();
}

CO_NAMESPACE_END