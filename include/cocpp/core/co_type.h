_Pragma("once");

#include "cocpp/core/co_define.h"

#include <condition_variable>
#include <mutex>
#include <set>
#include <vector>

CO_NAMESPACE_BEGIN

using co_byte = unsigned char;
using co_id = unsigned long long;
using co_tid = unsigned long long;
using co_pid = unsigned long long;

using co_buffer = std::vector<co_byte>;

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
    destroying,
    created,
};

CO_NAMESPACE_END
