#pragma once

class co_ctx;

using co_byte             = unsigned char;
using co_startup_functype = void (*)(co_ctx*);
using co_id               = unsigned long long;

enum class co_state : unsigned char
{
    created,   // 创建
    suspended, // 暂停
    running,   // 运行
    finished,  // 结束
};

enum class co_env_state : unsigned char
{
    idle,       // 空闲状态，此时可以将ctx加入到env中，或者释放此env
    busy,       // 繁忙状态，可以将ctx加入到env
    blocked,    // 阻塞状态，需要创建更多的env分担任务
    destorying, // 正在销毁状态，不再调度ctx
    created,    // 创建完成状态，此env不能用于调度ctx，通常是普通线程适配产生的env
};