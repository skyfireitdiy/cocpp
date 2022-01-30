_Pragma("once");
#include "cocpp/core/co_define.h"
#include "cocpp/core/co_type.h"

#include <atomic>
CO_NAMESPACE_BEGIN

class co_env;
class co_stack;
class co_env;

void     switch_to(co_byte** curr_regs, co_byte** next_regs); // 切换上下文
void     init_ctx(co_stack* stack, co_ctx* ctx);              // 初始化ctx
co_byte* get_rsp(co_ctx* ctx);                                // 获取ctx的rsp
co_tid   gettid();                                            // 获取当前线程id
void     setup_switch_handler();                              // 设置切换上下文的处理函数
void     send_switch_from_outside_signal(co_env* env);        // 发送切换上下文的信号

CO_NAMESPACE_END
