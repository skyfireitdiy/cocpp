// 此 example 演示固定缓冲大小channel的使用

#include "cocpp/cocpp.h"

void producer(cocpp::co_chan<int>& chan)
{
    for (int i = 0; i < 10000; ++i)
    {
        printf("producer: %d\n", i);
        chan << i;
    }
    chan.close();
}

void customer(cocpp::co_chan<int>& chan)
{
    int sum = 0;
    for (auto i : chan)
    {
        printf("customer: %d\n", i);
        sum += i;
    }
    printf("sum: %d\n", sum);
}

int main()
{
    cocpp::co_chan<int> chan(100);
    cocpp::co                co_producer(&producer, std::ref(chan));
    cocpp::co                co_customer(&customer, std::ref(chan));
    co_producer.join();
    co_customer.join();
}