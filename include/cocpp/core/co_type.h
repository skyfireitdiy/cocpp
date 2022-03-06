_Pragma("once");

#include "cocpp/core/co_define.h"

#include <condition_variable>
#include <mutex>
#include <set>

CO_NAMESPACE_BEGIN

class co_ctx;
class co_env;
class co_scheduler_factory;
class co_env_factory;
class co_ctx_factory;
class co_stack_factory;

using co_byte         = unsigned char;
using co_id           = unsigned long long;
using co_event_handle = unsigned long long;
using co_tid          = unsigned long long;
using co_timer_handle = unsigned long long;

enum class co_state : unsigned char
{
    suspended,
    running,
    finished,
};

enum class co_env_state : unsigned char
{
    idle,
    busy,
    blocked,
    destorying,
    created,
};

struct co_env_set
{
    std::set<co_env*>           normal_set;
    std::recursive_mutex        normal_lock;
    std::set<co_env*>           expired_set;
    std::recursive_mutex        expired_lock;
    std::condition_variable_any cv_expired_env;
    unsigned int                normal_env_count;
    std::recursive_mutex        mu_normal_env_count;
    unsigned int                base_env_count;
    unsigned int                max_env_count;
};

CO_NAMESPACE_END