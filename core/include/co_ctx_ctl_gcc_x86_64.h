#pragma once
#include "co_ctx.h"
#include "co_define.h"
#include "co_stack.h"
#include "co_type.h"

#ifdef __GNUC__
#ifdef __x86_64__

CO_NAMESPACE_BEGIN

void switch_to__(co_byte** curr_regs, co_byte** next_regs);
void init_ctx__(co_stack* shared_stack, co_ctx* ctx);

static constexpr int reg_index_RDI__   = 0;
static constexpr int reg_index_RIP__   = 1;
static constexpr int reg_index_RSP__   = 2;
static constexpr int reg_index_RBP__   = 3;
static constexpr int reg_index_RBX__   = 4;
static constexpr int reg_index_R12__   = 5;
static constexpr int reg_index_R13__   = 6;
static constexpr int reg_index_R14__   = 7;
static constexpr int reg_index_R15__   = 8;
static constexpr int reg_index_MXCSR__ = 9;
static constexpr int reg_index_FCW__   = 10;

CO_NAMESPACE_END

#endif
#endif