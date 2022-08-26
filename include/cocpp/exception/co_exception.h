_Pragma("once");

#include "cocpp/core/co_define.h"
#include "cocpp/core/co_vos.h"
#include <functional>
#include <signal.h>
#include <string>
#include <ucontext.h>
#include <vector>

CO_NAMESPACE_BEGIN

std::string backtrace();
std::string maps_info();
std::string time_info();
std::string regs_info(const ucontext_t& ctx);
std::string dump_memory(const co_byte* addr, size_t size);

void print_debug_info(const std::string& item_name, std::function<std::string()> f);

void set_up_signal_handler(const std::vector<int>& signals);
void signal_handler(int sig_no, siginfo_t* info, void* context);

CO_NAMESPACE_END