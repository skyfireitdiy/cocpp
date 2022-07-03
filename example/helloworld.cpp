#include "cocpp/cocpp.h"

#include <cstdio>
#include <string>

int main()
{
    cocpp::co c1([]() {
        return std::string("Hello ");
    });

    auto ret = c1.wait<std::string>();
    printf("%s", (ret + "World!\n").c_str());
}