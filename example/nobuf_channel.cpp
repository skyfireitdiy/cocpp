// 此 example 演示无缓冲channel的使用

#include "cocpp/comm/co_chan.h"
#include "cocpp/interface/co.h"

void producer(cocpp::co_chan<int, 0>& chan)
{
    for (int i = 0; i < 10000; ++i)
    {
        printf("producer: %d\n", i);
        chan << i;
    }
    chan.close();
}

void customer(cocpp::co_chan<int, 0>& chan)
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
    cocpp::co_chan<int, 0> chan;
    cocpp::co              co_producer(&producer, std::ref(chan));
    cocpp::co              co_customer(&customer, std::ref(chan));
    co_producer.join();
    co_customer.join();
}