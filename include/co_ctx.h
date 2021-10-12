#pragma once
#include "co_ctx.h"
#include "co_ctx_config.h"
#include "co_event.h"
#include "co_flag_manager.h"
#include "co_stack.h"
#include "co_type.h"

#include <any>
#include <atomic>
#include <bitset>
#include <mutex>

CO_NAMESPACE_BEGIN

class co_env;

class co_ctx final : public co_nocopy,
                     public co_flag_manager<CO_CTX_FLAG_MAX>
{
    RegCoEvent(co_finished, co_ctx*);

private:
    co_stack*             stack__ { nullptr };           // 当前栈空间
    std::atomic<co_state> state__ { co_state::created }; // 协程状态
    co_ctx_config         config__ {};                   // 协程配置
    std::any              ret__;                         // 协程返回值，会被传递给 config 中的 entry
    co_env*               env__ { nullptr };             // 协程当前对应的运行环境

    std::atomic<int> priority__ { CO_IDLE_CTX_PRIORITY }; // 优先级

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

#ifdef __GNUC__
#ifdef __x86_64__

    co_byte* regs__[11] {};
#else
#error only supported x86_64
#endif
#endif

    co_ctx(co_stack* stack, const co_ctx_config& config);

public:
    co_stack*            stack() const;
    co_state             state() const;
    co_byte**            regs();
    void                 set_state(co_state state);
    const co_ctx_config& config() const;
    std::any&            ret_ref();
    void                 set_env(co_env* env);
    co_env*              env() const;
    void                 set_priority(int priority);
    int                  priority() const;
    bool                 can_destroy() const;
    void                 lock_destroy();
    void                 unlock_destroy();

    friend void co_entry(co_ctx* ctx);
    friend class co_ctx_factory;
};

CO_NAMESPACE_END