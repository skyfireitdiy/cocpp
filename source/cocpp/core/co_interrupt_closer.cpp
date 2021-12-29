#include "cocpp/core/co_interrupt_closer.h"
#include "cocpp/core/co_vos.h"

CO_NAMESPACE_BEGIN

co_interrupt_closer::co_interrupt_closer()
{
    lock_interrupt();
    increate_interrupt_lock_count();
    unlock_interrupt();
}

co_interrupt_closer::~co_interrupt_closer()
{
    lock_interrupt();
    decreate_interrupt_lock_count();
    unlock_interrupt();
}

CO_NAMESPACE_END