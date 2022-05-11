#include "cocpp/interface/co_pipeline.h"
#include <chrono>
#include <iostream>
#include <vector>

using namespace cocpp;
using namespace std;

int nnnn = 0;

int main()
{
    auto ch = co_pipeline({ 0, 1, 2, 3, 4, 5 })
              | [](int a) {
                    printf("0x%x %d+1\n", gettid(), a + 1);
                    this_co::sleep_for(10ms);
                    return a + 1;
                }
              | [](int a) {
                    printf("0x%x %d*2\n", gettid(), a * 2);
                    this_co::sleep_for(10ms);
                    return a * 2;
                }
              | pipeline::filter<int>([](int n) { return n % 3 == 0; }) | pipeline::to<vector<int>>();
    for (auto t : ch)
    {
        cout << t << endl;
    }
}