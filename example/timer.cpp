#include "cocpp/core/co_timer.h"
#include "cocpp/interface/co.h"

using namespace cocpp;

int main()
{
    auto timer = co_timer::create([]() { printf("hello world\n"); }, co_expire_type::once, 1000);
    timer->start();
    this_co::sleep_for(std::chrono::milliseconds(2000));
}