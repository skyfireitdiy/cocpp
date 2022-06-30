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


stream_t stream(int size) { return stream_t { .size__ = size }; }

}

CO_NAMESPACE_END