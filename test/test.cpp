
#include "cocpp/interface/co.h"
#include <gtest/gtest.h>

int main(int argc, char** argv)
{
    cocpp::co::set_base_schedule_thread_count(1);
    cocpp::co::set_max_schedule_thread_count(1);
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
