_Pragma("once");

#include "cocpp/core/co_define.h"
#include "cocpp/core/co_vos.h"
#include "cocpp/exception/co_exception_iface.h"
#include <functional>
#include <signal.h>
#include <string>
#include <ucontext.h>
#include <vector>

CO_NAMESPACE_BEGIN

std::string regs_info(const ucontext_t& ctx);
std::string dump_memory(const co_byte* addr, size_t size);
std::string backtrace();
std::string maps_info();
std::string time_info();

void set_up_signal_handler(const std::vector<int>& signals);

CO_NAMESPACE_END