#include "cocpp/exception/co_exception.h"
#include "cocpp/core/co_define.h"
#include "cocpp/core/co_manager.h"
#include "cocpp/core/co_vos.h"
#include <execinfo.h>
#include <iomanip>
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

void handle_exception(ucontext_t& context, int sig)
{
    fprintf(stderr, "signal %d\n", sig);
    static bool in_exception = false;
    if (in_exception)
    {
        fprintf(stderr, "exception occurs in exception handler\n");
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

void set_up_signal_handler(const std::vector<int>& signals)
{
    for (auto&& s : signals)
    {
        struct sigaction sa;
        sa.sa_sigaction = signal_handler;
        sa.sa_flags     = SA_SIGINFO | SA_ONSTACK;

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

void signal_handler(int sig_no, siginfo_t* info, void* ctx)
{
    auto* context = reinterpret_cast<ucontext_t*>(ctx);
    if (sig_no == CO_SWITCH_SIGNAL)
    {
        switch_from_outside(*context);
    }
    else
    {
        handle_exception(*context, sig_no);
    }
}

std::string regs_info(const ucontext_t& ctx)
{
    stringstream ss;

    // todo 打印寄存器信息

    return ss.str();
}

void print_debug_info(const std::string& item_name, std::function<std::string()> f)
{
    fprintf(stderr, "=================== %s start ===================\n", item_name.c_str());
    fprintf(stderr, "%s\n", f().c_str());
    fprintf(stderr, "=================== %s end ===================\n", item_name.c_str());
}

std::string dump_memory(const co_byte* addr, size_t size)
{
    stringstream ss;
    for (size_t i = 0; i < size; i += sizeof(uint64_t))
    {
        if (i % (4 * sizeof(uint64_t)) == 0)
        {
            ss << "0x" << hex << setw(16) << setfill('0') << (void*)(addr + i)
               << ": ";
        }
        ss << hex << setw(16) << setfill('0') << (*(uint64_t*)(addr + i))
           << " ";
        if (i % (4 * sizeof(uint64_t)) == 3 * sizeof(uint64_t))
        {
            ss << endl;
        }
    }
    return ss.str();
}

CO_NAMESPACE_END