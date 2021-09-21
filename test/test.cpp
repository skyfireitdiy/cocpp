#if 0

#include <gtest/gtest.h>

int main(int argc, char** argv)
{
    testing::InitGoogleTest(&argc, argv);
    RUN_ALL_TESTS();
}

#else
#include "co.h"
#include "co_default_ctx_factory.h"
#include "co_default_env_factory.h"
#include "co_default_manager.h"
#include "co_default_stack_factory.h"
#include "co_o1_scheduler_factory.h"

#include <thread>

int main()
{
    co::init_co(co_default_manager::instance(co_o1_scheduler_factory::instance(),
                                             co_default_stack_factory::instance(),
                                             co_default_ctx_factory::instance(),
                                             co_default_env_factory::instance()));
    co c1([]() {});
    // std::thread th([]() {
    //     co::convert_to_schedule_thread();
    // });
    co c2([]() {});
    c1.wait<void>();
    c2.wait<void>();
    co::uninit_co();
    // th.join();
}

#endif
