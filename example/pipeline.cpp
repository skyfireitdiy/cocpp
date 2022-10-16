#include "cocpp/cocpp.h"
#include <iostream>

using namespace cocpp;
using namespace std;

int main()
{
    int source = 0;
    auto ch = co_pipeline<int>([&source]() -> std::optional<int> { // 生成[0, 1000000) 序列
                  if (source < 1000000)
                  {
                      return source++;
                  }
                  return std::nullopt;
              })
              | pipeline::take(2000)                  // 取前 2000 项
              | pipeline::skip(1000)                  // 跳过前 1000 项 （相当于取[1000, 1999)）
              | pipeline::fork(10, [](int n) -> int { // 每个数字乘以2 ，10个协程同时计算
                    return n * 2;
                })
              | pipeline::filter([](int n) {
                    return n % 3 == 0;
                }) // 过滤出 3 的倍数
              | pipeline::reduce([](int n, int m) {
                    return n + m;
                },
                                 0) // 求和
              | pipeline::chan();   // 转换为 channel

    for (auto t : ch)
    {
        cout << t << endl;
    }
}