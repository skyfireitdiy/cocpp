#include "cocpp/exception/co_exception.h"
#include "cocpp/core/co_define.h"
#include "cocpp/core/co_manager.h"
#include "cocpp/core/co_vos.h"
#include <cstdio>
#include <cstdlib>
#include <execinfo.h>
#include <iomanip>
#include <signal.h>
#include <sstream>
#include <unistd.h>

using namespace std;

CO_NAMESPACE_BEGIN

static FILE *exec_file = stderr;

static string exe_path();
static void print_debug_info(const string &item_name, function<string()> f);
static void signal_handler(int signo) __attribute__((optimize("O0"))); // 信号处理函数不能优化，否则获取的寄存器信息有问题

static string exe_path()
{
    char buf[1024];
    char *ret = realpath("/proc/self/exe", buf);
    if (ret != nullptr)
    {
        return "";
    }
    return string(buf);
}

FILE *set_exec_file(FILE *file)
{
    auto old = exec_file;
    exec_file = file;
    return old;
}

string backtrace()
{
    vector<void *> buffer(100);
    int pointer_num = ::backtrace(buffer.data(), buffer.size());
    char **string_buffer = ::backtrace_symbols(buffer.data(), pointer_num);
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

void handle_exception(sigcontext_64 *context, int sig)
{
    fprintf(exec_file, "signal %d\n", sig);
    static bool in_exception = false;
    if (in_exception)
    {
        fprintf(exec_file, "exception occurs in exception handler\n");
        exit(128 - sig);
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
    exit(128 - sig);
}

void set_up_signal_handler(const vector<int> &signals)
{
    for (auto &&s : signals)
    {
        struct sigaction sa;
        sa.sa_handler = signal_handler;

        sigemptyset(&sa.sa_mask);
        sa.sa_flags = 0;
        sigaction(s, &sa, nullptr);
    }
}

string time_info()
{
    time_t t = time(nullptr);
    char buf[64];
    strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", localtime(&t));
    return buf;
}

string maps_info()
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
    FILE *fp = fopen("/proc/self/maps", "r");
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
    sigcontext_64 *context = nullptr;
    __asm volatile("leaq 88(%%rsp), %%rax\t\n"
                   "movq %%rax, %0\t\n"
                   : "=m"(context)
                   :
                   : "memory", "rax");
    if (signo == CO_SWITCH_SIGNAL)
    {
        switch_from_outside(context);
    }
    else if (signo == SIGPIPE)
    {
        // 忽略SIGPIPE
        return;
    }
    else
    {
        handle_exception(context, signo);
    }
}

string regs_info(const sigcontext_64 *ctx)
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

void print_debug_info(const string &item_name, function<string()> f)
{
    fprintf(exec_file, "=================== %s start ===================\n", item_name.c_str());
    fprintf(exec_file, "%s\n", f().c_str());
    fprintf(exec_file, "=================== %s end ===================\n", item_name.c_str());
}

string dump_memory(const co_byte *addr, size_t size)
{
    stringstream ss;
    for (size_t i = 0; i < size; i += sizeof(uint64_t))
    {
        if (i % (4 * sizeof(uint64_t)) == 0)
        {
            ss << "0x" << hex << setw(16) << setfill('0') << (void *)(addr + i)
               << ": ";
        }
        ss << hex << setw(16) << setfill('0') << (*(uint64_t *)(addr + i))
           << " ";
        if (i % (4 * sizeof(uint64_t)) == 3 * sizeof(uint64_t))
        {
            ss << endl;
        }
    }
    return ss.str();
}

CO_NAMESPACE_END
