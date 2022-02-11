_Pragma("once");

#include "cocpp/core/co_define.h"

#include <condition_variable>
#include <functional>
#include <mutex>

CO_NAMESPACE_BEGIN

class co_sleep_controller final
{
private:
    std::recursive_mutex        mu__;      // 互斥锁
    std::condition_variable_any cv__;      // 条件变量
    std::function<bool()>       checker__; // 条件检查函数
public:
    co_sleep_controller(std::function<bool()> checker);                 // 构造函数
    void                  wake_up();                                    // 唤醒
    void                  sleep_if_need();                              // 如果需要睡眠则睡眠
    void                  sleep_if_need(std::function<bool()> checker); // 如果需要睡眠则睡眠，自定义条件检查
    std::recursive_mutex& sleep_lock();                                 // 获取睡眠锁
};

CO_NAMESPACE_END