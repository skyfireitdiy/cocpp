#include "co_stack_mem_pool.h"
#include "co_define.h"
#include "co_type.h"
#include <bitset>
#include <cassert>
#include <cmath>
#include <cstddef>
#include <new>

CO_NAMESPACE_BEGIN

co_stack_mem_pool::co_stack_mem_pool(size_t min_zone, size_t zone_count)
    : min_zone__(min_zone)
    , zone_count__(zone_count)
    , mem_pool__(zone_count)
{
}

unsigned long long co_stack_mem_pool::align_2_zone_size__(unsigned long long size)
{
    std::bitset<sizeof(size) * 8> bits(size);
    int                           first_one = 0;
    int                           last_one  = 0;
    for (size_t i = 0; i < sizeof(size) * 8; ++i)
    {
        if (bits.test(i))
        {
            first_one = first_one == 0 ? i : first_one;
            last_one  = i;
        }
    }
    if (last_one == first_one)
    {
        return last_one;
    }
    return last_one + 1;
}

unsigned long long co_stack_mem_pool::align_size__(unsigned long long size)
{
    return (size + sizeof(void*) - 1) / sizeof(void*) * sizeof(void*);
}

size_t co_stack_mem_pool::get_zone_index__(size_t size) const
{
    unsigned long long zone = align_2_zone_size__(size);
    if (zone < min_zone__)
    {
        zone = min_zone__;
    }
    return zone - min_zone__;
}

co_byte* co_stack_mem_pool::alloc_mem(size_t size)
{
    size_t zone_index = get_zone_index__(size);
    if (zone_index >= zone_count__)
    {
        return reinterpret_cast<co_byte*>(std::aligned_alloc(sizeof(void*), align_size__(size)));
    }

    if (mem_pool__[zone_index].empty())
    {
        return reinterpret_cast<co_byte*>(std::aligned_alloc(sizeof(void*), 1 << (zone_index + min_zone__)));
    }
    auto ret = mem_pool__[zone_index].front();
    mem_pool__[zone_index].pop_front();
    return ret;
}

void co_stack_mem_pool::free_mem(co_byte* ptr, size_t size)
{
    size_t zone_index = get_zone_index__(size);
    if (zone_index >= zone_count__)
    {
        std::free(ptr);
        return;
    }
    mem_pool__[zone_index].push_front(ptr);
}

void co_stack_mem_pool::free_pool()
{
    for (auto& zone : mem_pool__)
    {
        for (auto& mem : zone)
        {
            std::free(mem);
        }
        zone.clear();
    }
}

co_stack_mem_pool::~co_stack_mem_pool()
{
    free_pool();
}

CO_NAMESPACE_END