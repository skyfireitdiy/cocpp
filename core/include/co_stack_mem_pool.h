#pragma once

#include "co_define.h"
#include "co_type.h"
#include <list>
#include <vector>

CO_NAMESPACE_BEGIN

class co_stack_mem_pool final
{
private:
    static unsigned long long align_2_zone_size__(unsigned long long size);
    static unsigned long long align_size__(unsigned long long size);

    size_t get_zone_index__(size_t size) const;

    const size_t                     min_zone__;
    const size_t                     zone_count__;
    std::vector<std::list<co_byte*>> mem_pool__;

public:
    co_stack_mem_pool(size_t min_zone = MIN_STACK_MEM_ZONE, size_t zone_count = MAX_STACK_ZONE_COUNT);
    co_byte* alloc_mem(size_t size);
    void     free_mem(co_byte* ptr, size_t size);
    void     free_pool();
    ~co_stack_mem_pool();
};

CO_NAMESPACE_END