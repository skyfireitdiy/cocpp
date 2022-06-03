_Pragma("once");

#include "cocpp/core/co_define.h"
#include "cocpp/core/co_vos.h"
#include <functional>
#include <string>
#include <vector>

CO_NAMESPACE_BEGIN

struct sigcontext_64
{
    unsigned long long r8;    // 0*8
    unsigned long long r9;    // 1*8
    unsigned long long r10;   // 2*8
    unsigned long long r11;   // 3*8
    unsigned long long r12;   // 4*8
    unsigned long long r13;   // 5*8
    unsigned long long r14;   // 6*8
    unsigned long long r15;   // 7*8
    unsigned long long di;    // 8*8
    unsigned long long si;    // 9*8
    unsigned long long bp;    // 10*8
    unsigned long long bx;    // 11*8
    unsigned long long dx;    // 12*8
    unsigned long long ax;    // 13*8
    unsigned long long cx;    // 14*8
    unsigned long long sp;    // 15*8
    unsigned long long ip;    // 16*8
    unsigned long long flags; // 17*8

    // 后面这部分获取不到，暂时不用

    // unsigned short     cs;           //18*8
    // unsigned short     gs;           //18*8+2
    // unsigned short     fs;           //18*8+4
    // unsigned short     __pad0;       //18*8+6
    // unsigned long long err;          //19*8
    // unsigned long long trapno;       //20*8
    // unsigned long long oldmask;      //21*8
    // unsigned long long cr2;          //22*8
    // unsigned long long fpstate;      //23*8
    // unsigned long long reserved1[8]; //24*8
};

std::string backtrace();
std::string maps_info();
std::string time_info();
std::string regs_info(const sigcontext_64* ctx);

void print_debug_info(const std::string& item_name, std::function<std::string()> f);

void set_up_signal_handler(const std::vector<int>& signals);
void signal_handler(int signo);

CO_NAMESPACE_END