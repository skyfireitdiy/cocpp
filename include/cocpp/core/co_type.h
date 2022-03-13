_Pragma("once");

#include "cocpp/core/co_define.h"

#include <condition_variable>
#include <mutex>
#include <set>

CO_NAMESPACE_BEGIN

using co_byte = unsigned char;
using co_id   = unsigned long long;
using co_tid  = unsigned long long;

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

CO_NAMESPACE_END