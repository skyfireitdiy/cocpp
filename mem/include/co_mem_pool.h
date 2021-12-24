_Pragma("once");

#include "co_define.h"
#include "co_noncopyable.h"
#include "co_spinlock.h"
#include "co_type.h"
#include <deque>
#include <vector>

CO_NAMESPACE_BEGIN

class co_mem_pool final : private co_noncopyable
{
private:
    static unsigned long long         align_2_zone_edge__(unsigned long long size); // 对齐到zone边界
    static unsigned long long         align_size__(unsigned long long size);        // 对齐到8字节
    size_t                            get_zone_index__(size_t size) const;          // 获取zone的索引
    co_spinlock                       mu__ { co_spinlock::lock_type::in_thread };   // 互斥锁
    const size_t                      min_zone__;                                   // 最小zone
    const size_t                      zone_count__;                                 // zone数量
    const size_t                      max_cap__;                                    // 最大容量
    std::vector<std::deque<co_byte*>> mem_pool__;                                   // 内存池
public:                                                                             //
    co_mem_pool(size_t min_zone, size_t zone_count, size_t max_cap);                // 构造函数
    co_byte* alloc_mem(size_t size);                                                // 分配内存
    void     free_mem(co_byte* ptr, size_t size);                                   // 释放内存
    void     free_pool();                                                           // 释放内存池
    ~co_mem_pool();                                                                 // 析构函数
};

CO_NAMESPACE_END