// 此 example 演示死循环强制调度

#include "cocpp/cocpp.h"

#include <thread>

bool stop_flag = false;

void deadloop()
{
    while (!stop_flag)
    {
        using namespace std::chrono_literals;
        std::this_thread::sleep_for(1ms);
        printf("deadloop\n");
    }
    printf("deadloop exit\n");
}

void stop()
{
    printf("set stop flag\n");
    stop_flag = true;
}

int main()
{
    using namespace std::chrono_literals;
    auto      env = cocpp::co::create_env(true);
    cocpp::co co_deadloop({ cocpp::with_bind_env(env) }, &deadloop);
    std::this_thread::sleep_for(1s);
    cocpp::co co_stop({ cocpp::with_bind_env(env) }, &stop);

    co_deadloop.join();
    co_stop.join();
}