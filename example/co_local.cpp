// 此 example 演示协程私有数据

#include "cocpp/interface/co.h"

#include <cstdio>

void sum(int n)
{
    for (int i = 0; i < n; ++i)
    {
        CoLocal(sum, int) += i;
    }
    printf("sum: %d\n", CoLocal(sum, int));
}

int main()
{
    cocpp::co co_sum1(&sum, 100);
    cocpp::co co_sum2(&sum, 1000);
    co_sum1.join();
    co_sum2.join();
}