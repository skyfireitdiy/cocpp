#if 1

#include "co.h"
#include "co_o1_scheduler_factory.h"
#include <gtest/gtest.h>

class CoEnvironment : public testing::Environment
{
public:
    void SetUp() override
    {
        co::init_co(co_o1_scheduler_factory::instance());
    }
    void TearDown() override
    {
        co::uninit_co();
    }
};

int main(int argc, char** argv)
{
    testing::AddGlobalTestEnvironment(new CoEnvironment);
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

#else
#include "co.h"
#include "co_ctx_factory.h"
#include "co_env_factory.h"
#include "co_manager.h"
#include "co_o1_scheduler_factory.h"
#include "co_stack_factory.h"

#include <thread>

int main()
{
    co::init_co(co_manager::instance(co_o1_scheduler_factory::instance(),
                                     co_stack_factory::instance(),
                                     co_ctx_factory::instance(),
                                     co_env_factory::instance()));
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
