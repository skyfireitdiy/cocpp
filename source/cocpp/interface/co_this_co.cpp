#include "cocpp/interface/co_this_co.h"
#include "cocpp/core/co_vos.h"
#include "cocpp/interface/co.h"

CO_NAMESPACE_BEGIN

namespace this_co
{

void yield()
{
    co::current_env()->schedule_switch();
}

co_id id()
{
    return reinterpret_cast<co_id>(co::current_ctx());
}

std::string name()
{
    return co::current_ctx()->config().name;
}

}

CO_NAMESPACE_END