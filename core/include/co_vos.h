#pragma once
#include "co_ctx.h"
#include "co_define.h"
#include "co_stack.h"
#include "co_type.h"

CO_NAMESPACE_BEGIN

void     switch_to(co_byte** curr_regs, co_byte** next_regs);
void     init_ctx(co_stack* shared_stack, co_ctx* ctx);
co_byte* get_rsp(co_ctx* ctx);
co_tid   gettid();

CO_NAMESPACE_END
