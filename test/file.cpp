#include <gtest/gtest.h>
#include "cocpp/io/co_file.h"

using namespace cocpp;

TEST(file, open_file)
{
    co_file f;
    int ret = f.open("/dev/null", O_RDONLY, 0);
    EXPECT_EQ(ret, 0);
    f.close();
}
