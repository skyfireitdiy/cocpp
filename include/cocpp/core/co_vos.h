_Pragma("once");
#include "cocpp/core/co_define.h"
#include "cocpp/core/co_type.h"

#include <atomic>
CO_NAMESPACE_BEGIN

class co_env;
class co_stack;
class co_env;

void     switch_to(co_byte** curr_regs, co_byte** next_regs); // 切换上下文
void     init_ctx(co_stack* shared_stack, co_ctx* ctx);       // 初始化ctx
co_byte* get_rsp(co_ctx* ctx);                                // 获取ctx的rsp
co_tid   gettid();                                            // 获取当前线程id
void     setup_switch_handler();                              // 设置切换上下文的处理函数
void     send_switch_from_outside_signal(co_env* env);        // 发送切换上下文的信号
bool     can_interrupt();                                     // 是否可中断
void     lock_interrupt();                                    // 加锁中断
void     unlock_interrupt();                                  // 解锁中断
void     increate_interrupt_lock_count();                     // 增加中断次数
void     decreate_interrupt_lock_count();                     // 减少中断次数

CO_NAMESPACE_END
