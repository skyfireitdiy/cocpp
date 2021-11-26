_Pragma("once");
#include "co_define.h"
#include "co_type.h"

CO_NAMESPACE_BEGIN

class co_env;
class co_stack;
class co_env;

void     switch_to(co_byte** curr_regs, co_byte** next_regs);
void     init_ctx(co_stack* shared_stack, co_ctx* ctx);
co_byte* get_rsp(co_ctx* ctx);
co_tid   gettid();
void     setup_switch_handler();
void     send_switch_from_outside_signal(co_env* env);

CO_NAMESPACE_END
