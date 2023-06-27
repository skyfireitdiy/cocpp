_Pragma("once");
#include "co_define.h"
#include "cocpp/comm/co_event.h"
#include "cocpp/core/co_ctx_config.h"
#include "cocpp/core/co_local.h"
#include "cocpp/core/co_type.h"
#include "cocpp/utils/co_any.h"
#include "cocpp/utils/co_flag_manager.h"
#include "cocpp/utils/co_state_manager.h"

#include <exception>
#include <memory>
#include <mutex>

CO_NAMESPACE_BEGIN

class co_env;
class co_stack;

class co_ctx final : private co_noncopyable
{
    RegCoEvent(finished);
    RegCoEvent(priority_changed, int, int);

private:
    co_flag_manager<CO_CTX_FLAG_MAX> flag_manager__;

    // In some scenarios, ensure that the status does not change to `finished`
    co_state_manager<co_state, co_state::suspended, co_state::finished> state_manager__;
    std::recursive_mutex finish_lock__;

    struct
    {
        std::recursive_mutex mu;
        std::set<void *> resource;
    } wait_data__ {};

    co_stack *stack__ {nullptr};

    co_ctx_config config__ {};
    co_any ret__;

    // Other threads may change the priority
    int priority__ {CO_IDLE_CTX_PRIORITY};
    mutable std::recursive_mutex priority_lock__;

    co_env *env__ {nullptr};

    std::map<std::string, std::shared_ptr<co_local_base> > locals__;
    const std::function<void(co_any &)> entry__;
    std::exception_ptr exception__;

#ifdef __GNUC__
#ifdef __x86_64__
    co_byte *regs__[32] {};
#else
#error only supported x86_64
#endif
#endif
    co_ctx(co_stack *stack, const co_ctx_config &config, std::function<void(co_any &)> entry);

public:
    void set_priority(int priority);
    size_t priority() const;
    bool can_schedule() const;
    co_stack *stack() const;
    co_byte **regs();
    const co_ctx_config &config() const;
    co_any &ret_ref();
    void set_env(co_env *env);
    co_env *env() const;
    bool can_destroy() const;
    void lock_destroy();
    void unlock_destroy();
    void set_stack(co_stack *stack);
    bool can_move() const;
    std::string name() const;
    co_id id() const;
    void enter_wait_resource_state(void *rc);
    void leave_wait_resource_state(void *rc);
    std::function<void(co_any &)> entry() const;
    static void real_entry(co_ctx *ctx);
    void set_flag(size_t flag);
    void reset_flag(size_t flag);
    void check_and_rethrow_exception();
    void lock_finished_state();
    void unlock_finished_state();
    std::string ctx_info() const;
    void adjust_stack();

    template <typename T>
    T &local_storage(const std::string &name);

    CoConstMemberMethodProxy(&flag_manager__, test_flag);
    CoConstMemberMethodProxy(&state_manager__, state);
    CoMemberMethodProxy(&state_manager__, set_state);

    friend class co_ctx_factory;
};

// 模板实现

template <typename T>
T &co_ctx::local_storage(const std::string &name)
{
    if (!locals__.contains(name))
    {
        locals__[name] = std::make_shared<co_local<T> >();
    }
    return std::dynamic_pointer_cast<co_local<T> >(locals__[name])->get();
}

CO_NAMESPACE_END
