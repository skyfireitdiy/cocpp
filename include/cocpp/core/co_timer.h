_Pragma("once");

#include "cocpp/core/co_define.h"
#include "cocpp/utils/co_noncopyable.h"

#include <chrono>
#include <functional>
#include <mutex>

CO_NAMESPACE_BEGIN

using co_timer_handle = unsigned long long;

enum class co_timeout_type
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
    const std::function<void()>           callback__;
    const co_timeout_type                 timeout_type__;
    const co_timer_type                   timer_type__;
    const unsigned long long              interval__;
    std::chrono::steady_clock::time_point timeout_time__;
    co_timer_status                       status__;
    std::recursive_mutex                  mutex__;

    co_timer(const std::function<void()>& func, co_timeout_type type, unsigned long long interval_ms);

    co_timer(const std::function<void()>& func, std::chrono::steady_clock::time_point timeout_time);

    void insert_to_timer_queue__();
    void remove_from_timer_queue__();
    void update_timeout_time__();

public:
    ~co_timer();
    void            start();
    void            stop();
    void            reset();
    bool            is_running() const;
    co_timer_handle get_handle() const;

    static std::shared_ptr<co_timer> create(const std::function<void()>& func, co_timeout_type type, unsigned long long interval_ms);
};

bool operator==(const std::shared_ptr<co_timer>& lhs, const std::shared_ptr<co_timer>& rhs);
bool operator<(const std::shared_ptr<co_timer>& lhs, const std::shared_ptr<co_timer>& rhs);

CO_NAMESPACE_END