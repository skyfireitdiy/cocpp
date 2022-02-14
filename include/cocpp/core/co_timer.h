_Pragma("once");

#include "cocpp/core/co_define.h"
#include "cocpp/core/co_type.h"
#include "cocpp/utils/co_noncopyable.h"

#include <chrono>
#include <functional>
#include <memory>
#include <mutex>

CO_NAMESPACE_BEGIN

enum class co_expire_type
{
    once,
    loop,
};

enum class co_timer_type
{
    absolute,
    relative,
};

enum class co_timer_status
{
    stopped,
    running,
};

class co_timer final : private co_noncopyable,
                       public std::enable_shared_from_this<co_timer>
{
private:
    const co_timer_handle                 handle__;
    std::function<void()>                 callback__;
    const co_expire_type                  expire_type__;
    const co_timer_type                   timer_type__;
    const unsigned long long              interval__;
    std::chrono::steady_clock::time_point expire_time__;
    co_timer_status                       status__;
    mutable std::recursive_mutex          mutex__;

    co_timer(const std::function<void()>& func, co_expire_type type, unsigned long long interval_ms);
    co_timer(const std::function<void()>& func, std::chrono::steady_clock::time_point expire_time);

    void insert_to_timer_queue__();
    void remove_from_timer_queue__();
    void update_timeout_time__();

public:
    void                                  start();
    void                                  stop();
    void                                  reset();
    bool                                  is_running() const;
    co_timer_handle                       get_handle() const;
    std::chrono::steady_clock::time_point expire_time() const;
    void                                  run() const;
    co_expire_type                        expire_type() const;
    co_timer_type                         timer_type() const;
    bool                                  is_expired() const;
    std::function<void()>                 set_expire_callback(const std::function<void()>& func);

    static std::shared_ptr<co_timer>
    create(const std::function<void()>& func, co_expire_type type, unsigned long long interval_ms);

    static std::shared_ptr<co_timer>
    create(const std::function<void()>& func, std::chrono::steady_clock::time_point expire_time);
};

bool operator==(const std::shared_ptr<co_timer>& lhs, const std::shared_ptr<co_timer>& rhs);
bool operator<(const std::shared_ptr<co_timer>& lhs, const std::shared_ptr<co_timer>& rhs);

CO_NAMESPACE_END