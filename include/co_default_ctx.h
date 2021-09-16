#pragma once
#include "co_ctx.h"
#include "co_ctx_config.h"
#include "co_stack.h"

#include <any>
#include <atomic>
#include <bitset>
#include <mutex>

class co_default_ctx : public co_ctx
{
private:
    co_stack*             stack__;  // 当前栈空间
    std::atomic<co_state> state__;  // 协程状态
    co_ctx_config         config__; // 协程配置
    std::any              ret__;    // 协程返回值，会被传递给 config 中的 entry
    co_env*               env__;    // 协程当前对应的运行环境

    static constexpr int CO_CTX_FLAG_DETACHED = 0;
    static constexpr int CO_CTX_FLAG_MAX      = 8;

    std::bitset<CO_CTX_FLAG_MAX> flag__;    // 状态
    std::mutex                   mu_flag__; // 状态保护锁

#if defined(_MSC_VER)
#if defined(_WIN64)
    // 16字节浮点寄存器
    static constexpr int reg_index_XMM6__  = 0;
    static constexpr int reg_index_XMM7__  = 2;
    static constexpr int reg_index_XMM8__  = 4;
    static constexpr int reg_index_XMM9__  = 6;
    static constexpr int reg_index_XMM10__ = 8;
    static constexpr int reg_index_XMM11__ = 10;
    static constexpr int reg_index_XMM12__ = 12;
    static constexpr int reg_index_XMM13__ = 14;
    static constexpr int reg_index_XMM14__ = 16;
    static constexpr int reg_index_XMM15__ = 18;

    // 8字节普通寄存器
    static constexpr int reg_index_RCX__ = 20;
    static constexpr int reg_index_RIP__ = 21;
    static constexpr int reg_index_RBX__ = 22;
    static constexpr int reg_index_RBP__ = 23;
    static constexpr int reg_index_RDI__ = 24;
    static constexpr int reg_index_RSI__ = 25;
    static constexpr int reg_index_RSP__ = 26;
    static constexpr int reg_index_R12__ = 27;
    static constexpr int reg_index_R13__ = 28;
    static constexpr int reg_index_R14__ = 29;
    static constexpr int reg_index_R15__ = 30;

    // 这两个寄存器其实每个只有4字节
    static constexpr int reg_index_MXCSR__ = 32; // MXCSR Control and Status Register
    static constexpr int reg_index_FCW__   = 33; // x87 FPU Control Word

    // 进程及线程信息寄存器
    static constexpr int reg_index_TEB_8__    = 34;
    static constexpr int reg_index_TEB_16__   = 35;
    static constexpr int reg_index_TEB_24__   = 36;
    static constexpr int reg_index_TEB_5240__ = 37;

    // 保存xmm寄存器的地址要求16字节对齐
    alignas(16) co_byte* regs__[38];
#else
#error only supported x86_64
#endif
#endif

    void init_regs__();
    void set_flag__(int flag);
    bool test_flag__(int flag);
    void unset_flag__(int flag);
    co_default_ctx(co_stack* stack, const co_ctx_config& config);

public:
    co_stack*            stack() const override;
    co_state             state() const override;
    co_byte**            regs() override;
    void                 set_state(co_state state) override;
    const co_ctx_config& config() const override;
    std::any&            ret_ref() override;
    void                 set_env(co_env* env) override;
    co_env*              env() const override;
    void                 set_detach() override;
    bool                 detach() override;

    friend void co_default_entry(co_ctx* ctx);
    friend class co_default_ctx_factory;
};
