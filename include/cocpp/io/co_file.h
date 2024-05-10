_Pragma("once");

#include "cocpp/core/co_define.h"
#include "cocpp/io/co_io.h"

#include <optional>

CO_NAMESPACE_BEGIN

class co_file : public co_io
{
public:
    co_file() = default;

    static std::optional<co_file> open(const char *pathname, int flags, mode_t mode = 0);
};

CO_NAMESPACE_END
