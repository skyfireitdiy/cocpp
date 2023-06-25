_Pragma("once");
#include "cocpp/core/co_define.h"
#include "cocpp/core/co_type.h"

#include <atomic>
CO_NAMESPACE_BEGIN

class co_env;
class co_stack;
class co_ctx;
struct sigcontext_64;

void switch_to(co_byte **curr_regs, co_byte **next_regs);
void init_ctx(co_stack *stack, co_ctx *ctx);
co_byte *get_rsp(co_ctx *ctx);
co_tid gettid();
void send_switch_from_outside_signal(co_env *env);
void switch_from_outside(sigcontext_64 *context);
int set_mem_dontneed(void *ptr, size_t size);
void *alloc_mem_by_mmap(size_t size);
int free_mem_by_munmap(void *ptr, size_t size);

CO_NAMESPACE_END
