#include "cocpp/interface/co_pipeline.h"

CO_NAMESPACE_BEGIN

namespace pipeline
{
left_t left(size_t n)
{
    return left_t { .left__ = n };
}

not_left_t not_left(size_t n)
{
    return not_left_t { .not_left__ = n };
}
}

CO_NAMESPACE_END