#include "cocpp/sync/co_wait_group.h"
#include <cassert>

using namespace std;

CO_NAMESPACE_BEGIN

co_wait_group::co_wait_group(size_t count)
    : count__(count)
    , done__(0)
{
}

co_wait_group::~co_wait_group()
{
    wait();
}

void co_wait_group::done()
{
    scoped_lock lock(mutex__);
    ++done__;
    assert(done__ <= count__);
    if (done__ == count__)
    {
        cond__.notify_all();
    }
}

void co_wait_group::wait()
{
    unique_lock lock(mutex__);
    while (done__ < count__)
    {
        cond__.wait(lock);
    }
}

CO_NAMESPACE_END