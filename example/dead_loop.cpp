// 此 example 演示死循环强制调度

#include "cocpp/interface/co.h"

#include <thread>

bool stop_flag = false;

void deadloop()
{
    while (!stop_flag)
    {
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
    cocpp::co co_deadloop(&deadloop);
    std::this_thread::sleep_for(std::chrono::seconds(1));
    cocpp::co co_stop(&stop);

    co_deadloop.wait<void>();
    co_stop.wait<void>();
}