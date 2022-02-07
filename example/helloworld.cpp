#include "cocpp/interface/co.h"
#include "cocpp/interface/co_async_run.h"

#include <cstdio>
#include <string>

int main()
{
    auto c1 = cocpp::co_async_run([]() {
        return std::string("Hello ");
    });

    auto ret = c1.get();
    printf("%s", (ret + "World!\n").c_str());
}