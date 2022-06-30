#include "cocpp/interface/co_pipeline.h"

CO_NAMESPACE_BEGIN

namespace pipeline
{
take_t take(size_t n)
{
    return take_t { .take__ = n };
}

skip_t skip(size_t n)
{
    return skip_t { .skip__ = n };
}
}

CO_NAMESPACE_END