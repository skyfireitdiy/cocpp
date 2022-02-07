#include "cocpp/interface/co_async_run.h"

using namespace cocpp;

int add(int a, int b)
{
    return a + b;
}

int main()
{
    auto f = co_async_run(add, 5, 6);
    std::cout << f.get() << std::endl;
}