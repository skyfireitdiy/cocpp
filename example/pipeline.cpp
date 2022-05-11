#include "cocpp/interface/co_pipeline.h"

using namespace cocpp;

int nnnn = 0;

int main()
{
    auto ch = (co_pipeline<int, 1>([]() -> std::optional<int> {
                   if (nnnn < 5)
                   {
                       //    fprintf(stderr, "pipeline 1: %d\n", nnnn);
                       return nnnn++;
                   }
                   return std::nullopt;
               })
               | std::function<int(const int&)>([](int n) -> int {
                     //  fprintf(stderr, "pipeline 2: %d\n", n * 2);
                     return n * 2;
                 }))
                  .chan();

    // while (true)
    // {
    //     auto p = ch.pop();
    //     if (!p)
    //     {
    //         break;
    //     }
    //     fprintf(stderr, "pop -----: %d\n", *p);
    // }
    for (auto p : ch)
    {
        fprintf(stderr, "pipeline 3 ---------- : %d\n", p);
    }
}