_Pragma("once");

#include "cocpp/core/co_define.h"

CO_NAMESPACE_BEGIN

class co_interrupt_closer final
{
public:
    co_interrupt_closer();
    ~co_interrupt_closer();
};

CO_NAMESPACE_END