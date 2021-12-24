_Pragma("once");

#include "co_define.h"

#include <condition_variable>
#include <functional>
#include <mutex>

CO_NAMESPACE_BEGIN

class co_sleep_controller final
{
private:
    std::mutex              mu__;                       // 互斥锁
    std::condition_variable cond__;                     // 条件变量
    std::function<bool()>   checker__;                  // 条件检查函数
public:                                                 //
    co_sleep_controller(std::function<bool()> checker); // 构造函数
    void wake_up();                                     // 唤醒
    void sleep_if_need();                               // 如果需要睡眠则睡眠
};

CO_NAMESPACE_END