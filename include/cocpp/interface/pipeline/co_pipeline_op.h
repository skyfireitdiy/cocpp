_Pragma("once");

#include "cocpp/core/co_define.h"
#include "cocpp/interface/pipeline/co_pipeline_concept.h"

CO_NAMESPACE_BEGIN

///////////////////////////////////////////////////////////////////////
namespace pipeline
{
struct chan
{
};

template <Collection Type>
struct to
{
};

template <typename FilterType>
struct filter
{
    const FilterType filter__;
    filter(const FilterType& filter)
        : filter__(filter)
    {
    }
};

template <typename ReduceType, typename InitType>
struct reduce
{
    const ReduceType reducer__;
    const InitType   init__;
    reduce(const ReduceType& reducer, const InitType& init)
        : reducer__(reducer)
        , init__(init)
    {
    }
};

struct stream
{
    const int size__;
    stream(int size = -1)
        : size__(size)
    {
    }
};

struct take
{
    const size_t take__;
    take(const size_t& tk)
        : take__(tk)
    {
    }
};

struct skip
{
    const size_t skip__;
    skip(const size_t& sp)
        : skip__(sp)
    {
    }
};

template <typename FuncType>
struct fork
{
    const size_t   fork_count__;
    const FuncType func__;
    fork(const size_t& fc, const FuncType& func)
        : fork_count__(fc)
        , func__(func)
    {
    }
};

///////////////////////////////////////////////////////////////////////

}

CO_NAMESPACE_END