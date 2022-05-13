#include "cocpp/interface/co_pipeline.h"
#include <chrono>
#include <iostream>
#include <vector>

using namespace cocpp;
using namespace std;

int nnnn = 0;

int main()
{
    auto ch = std::list({ 0, 1, 2, 3, 4, 5 })
              | pipeline::pipe<1>()
              | [](int a) {
                    this_co::sleep_for(10ms);
                    return a + 1;
                }
              | [](int a) {
                    this_co::sleep_for(10ms);
                    return a * 2;
                }
              | pipeline::filter([](int n) { return n % 3 == 0; }) | pipeline::reduce([](int a, int b) { return a * b; }, 1) | pipeline::to<vector<int>>();

    for (auto t : ch)
    {
        cout << t << endl;
    }
}