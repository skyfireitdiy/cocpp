_Pragma("once");

#include "cocpp/core/co_define.h"
#include "cocpp/core/co_type.h"
#include "cocpp/utils/co_noncopyable.h"
#include <deque>
#include <mutex>
#include <vector>

CO_NAMESPACE_BEGIN

class co_mem_pool final : private co_noncopyable
{
private:
    static unsigned long long         align_2_zone_edge__(unsigned long long size); 
    static unsigned long long         align_size__(unsigned long long size);        
    size_t                            get_zone_index__(size_t size) const;          
    std::recursive_mutex              mu__;                                         
    const size_t                      min_zone__;                                   
    const size_t                      zone_count__;                                 
    const size_t                      max_cap__;                                    
    std::vector<std::deque<co_byte*>> mem_pool__;                                   
public:
    co_mem_pool(size_t min_zone, size_t zone_count, size_t max_cap); 
    co_byte* alloc_mem(size_t size);                                 
    void     free_mem(co_byte* ptr, size_t size);                    
    void     free_pool();                                            
    ~co_mem_pool();                                                  
};

CO_NAMESPACE_END