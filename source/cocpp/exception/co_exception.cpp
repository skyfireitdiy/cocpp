#include "cocpp/exception/co_exception.h"
#include "cocpp/core/co_define.h"
#include "cocpp/core/co_manager.h"
#include "cocpp/core/co_vos.h"
#include <execinfo.h>
#include <signal.h>
#include <sstream>
#include <unistd.h>

using namespace std;

CO_NAMESPACE_BEGIN

static std::string exe_path();

static std::string exe_path()
{
    char    buf[1024];
    ssize_t len = readlink("/proc/self/exe", buf, sizeof(buf));
    if (len == -1)
    {
        return "";
    }
    return std::string(buf, len);
}

string backtrace()
{
    vector<void*> buffer(100);
    int           pointer_num   = ::backtrace(buffer.data(), buffer.size());
    char**        string_buffer = ::backtrace_symbols(buffer.data(), pointer_num);
    if (string_buffer == NULL)
    {
        return {};
    }
    string ret;
    for (int i = 0; i < pointer_num; i++)
    {
        ret += string_buffer[i];
        ret += "\n";
    }
    free(string_buffer);
    return ret;
}

void handle_exception(sigcontext_64* context, int sig)
{
    fprintf(stderr, "signal %d\n", sig);
    static bool in_exception = false;
    if (in_exception)
    {
        fprintf(stderr, "exception occurs in exception handler\n");
        exit(1);
    }
    in_exception = true;
    print_debug_info("time info", time_info);
    print_debug_info("regs info", [context]() {
        return regs_info(context);
    });

    print_debug_info("maps info", maps_info);
    print_debug_info("backtrace", backtrace);
    print_debug_info("co info", []() {
        return co_manager::instance()->manager_info();
    });
    exit(1);
}

void set_up_signal_handler(const std::vector<int>& signals)
{
    for (auto&& s : signals)
    {
        struct sigaction sa;
        sa.sa_handler = signal_handler;

        sigemptyset(&sa.sa_mask);
        sa.sa_flags = 0;
        sigaction(s, &sa, nullptr);
    }
}

std::string time_info()
{
    time_t t = time(nullptr);
    char   buf[64];
    strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", localtime(&t));
    return buf;
}

std::string maps_info()
{
    stringstream ss;
    ss << "exe:" << exe_path() << "\n";
    ss << "pid: " << getpid() << "\n";
    ss << "ppid: " << getppid() << "\n";
    ss << "uid: " << getuid() << "\n";
    ss << "euid: " << geteuid() << "\n";
    ss << "gid: " << getgid() << "\n";
    ss << "egid: " << getegid() << "\n";
    ss << "maps:" << endl;
    FILE* fp = fopen("/proc/self/maps", "r");
    if (fp == nullptr)
    {
        return "";
    }
    char buf[1024];
    while (fgets(buf, sizeof(buf), fp) != nullptr)
    {
        ss << buf;
    }
    fclose(fp);
    return ss.str();
}



void signal_handler(int signo)
{
    sigcontext_64* context = nullptr;
    __asm volatile("leaq 88(%%rsp), %%rax\t\n"
                   "movq %%rax, %0\t\n"
                   : "=m"(context)
                   :
                   : "memory", "rax");
    if (signo == CO_SWITCH_SIGNAL)
    {
        switch_from_outside(context);
    }
    else
    {
        handle_exception(context, signo);
    }
}

std::string regs_info(const sigcontext_64* ctx)
{
    stringstream ss;
#define OUT_PUT_REG(name) ss << #name ": 0x" << hex << ctx->name << dec << "(" << ctx->name << ")" << endl

    OUT_PUT_REG(r8);
    OUT_PUT_REG(r9);
    OUT_PUT_REG(r10);
    OUT_PUT_REG(r11);
    OUT_PUT_REG(r12);
    OUT_PUT_REG(r13);
    OUT_PUT_REG(r14);
    OUT_PUT_REG(r15);
    OUT_PUT_REG(di);
    OUT_PUT_REG(si);
    OUT_PUT_REG(bp);
    OUT_PUT_REG(bx);
    OUT_PUT_REG(dx);
    OUT_PUT_REG(ax);
    OUT_PUT_REG(cx);
    OUT_PUT_REG(sp);
    OUT_PUT_REG(ip);
    OUT_PUT_REG(flags);

#undef OUT_PUT_REG

    return ss.str();
}

void print_debug_info(const std::string& item_name, std::function<std::string()> f)
{
    fprintf(stderr, "=================== %s start ===================\n", item_name.c_str());
    fprintf(stderr, "%s\n", f().c_str());
    fprintf(stderr, "=================== %s end ===================\n", item_name.c_str());
}

CO_NAMESPACE_END