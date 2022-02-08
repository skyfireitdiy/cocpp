#include <type_traits>
_Pragma("once");

#include "cocpp/core/co_define.h"
#include "cocpp/sync/co_condition_variable.h"
#include "cocpp/sync/co_mutex.h"
#include "cocpp/utils/co_noncopyable.h"

#include <chrono>
#include <exception>
#include <future>
#include <memory>

CO_NAMESPACE_BEGIN

struct _Nil
{
};

template <class _Ty>
class _Associated_state;

template <class _Ty>
struct _Deleter_base
{ // abstract base class for managing deletion of state objects
    virtual void _Delete(_Associated_state<_Ty>*) = 0;
    virtual ~_Deleter_base() noexcept {}
};

template <class _Ty, class _Derived, class _Alloc>
struct _State_deleter : _Deleter_base<_Ty>
{ // manage allocator and deletion state objects
    _State_deleter(const _Alloc& _Al)
        : _My_alloc(_Al)
    {
    }

    _State_deleter(const _State_deleter&) = delete;
    _State_deleter& operator=(const _State_deleter&) = delete;

    void _Delete(_Associated_state<_Ty>* _State) override;

    _Alloc _My_alloc;
};

template <class _Ty>
class _Associated_state
{ // class for managing associated synchronous state
public:
    using _State_type = _Ty;
    using _Mydel      = _Deleter_base<_Ty>;

    _Associated_state(_Mydel* _Dp = nullptr)
        : _Refs(1)
        , // non-atomic initialization
        _Exception()
        , _Retrieved(false)
        , _Ready(false)
        , _Ready_at_thread_exit(false)
        , _Has_stored_result(false)
        , _Running(false)
        , _Deleter(_Dp)
    {
        // TRANSITION: _Associated_state ctor assumes _Ty is default constructible
    }

    virtual ~_Associated_state() noexcept
    {
        // if (_Has_stored_result && !_Ready)
        // { // registered for release at thread exit
        //     _Cond._Unregister(_Mtx);
        // }
    }

    void _Retain()
    { // increment reference count
      // _MT_INCR(_Refs);
        ++_Refs;
    }

    void _Release()
    { // decrement reference count and destroy when zero
        if (--_Refs == 0)
        {
            _Delete_this();
        }
    }

private:
    unsigned long _Refs;

public:
    virtual void _Wait()
    { // wait for signal
        std::unique_lock<co_mutex> _Lock(_Mtx);
        _Maybe_run_deferred_function(_Lock);
        while (!_Ready)
        {
            _Cond.wait(_Lock);
        }
    }

    struct _Test_ready
    { // wraps _Associated_state
        _Test_ready(const _Associated_state* _St)
            : _State(_St)
        {
        }

        bool operator()() const
        { // test state
            return _State->_Ready != 0;
        }
        const _Associated_state* _State;
    };

    template <class _Rep, class _Per>
    std::future_status _Wait_for(const std::chrono::duration<_Rep, _Per>& _Rel_time)
    { // wait for duration
        std::unique_lock<co_mutex> _Lock(_Mtx);
        if (_Has_deferred_function())
        {
            return std::future_status::deferred;
        }

        if (_Cond.wait_for(_Lock, _Rel_time, _Test_ready(this)))
        {
            return std::future_status::ready;
        }

        return std::future_status::timeout;
    }

    template <class _Clock, class _Dur>
    std::future_status _Wait_until(const std::chrono::time_point<_Clock, _Dur>& _Abs_time)
    { // wait until time point
        std::unique_lock<co_mutex> _Lock(_Mtx);
        if (_Has_deferred_function())
        {
            return std::future_status::deferred;
        }

        if (_Cond.wait_until(_Lock, _Abs_time, _Test_ready(this)))
        {
            return std::future_status::ready;
        }

        return std::future_status::timeout;
    }

    virtual _Ty& _Get_value(bool _Get_only_once)
    {
        std::unique_lock<co_mutex> _Lock(_Mtx);
        if (_Get_only_once && _Retrieved)
        {
            throw std::future_error(std::future_errc::future_already_retrieved);
        }

        if (_Exception)
        {
            std::rethrow_exception(_Exception);
        }

        _Retrieved = true;
        _Maybe_run_deferred_function(_Lock);
        while (!_Ready)
        {
            _Cond.wait(_Lock);
        }

        if (_Exception)
        {
            std::rethrow_exception(_Exception);
        }

        return _Result;
    }

    void _Set_value(const _Ty& _Val, bool _At_thread_exit)
    { // store a result
        std::unique_lock<co_mutex> _Lock(_Mtx);
        _Set_value_raw(_Val, &_Lock, _At_thread_exit);
    }

    void _Set_value_raw(const _Ty& _Val, std::unique_lock<co_mutex>* _Lock,
                        bool _At_thread_exit)
    { // store a result while inside a locked block
        if (_Has_stored_result)
        {
            throw std::future_error(std::future_errc::promise_already_satisfied);
        }

        _Result = _Val;
        _Do_notify(_Lock, _At_thread_exit);
    }

    void _Set_value(_Ty&& _Val, bool _At_thread_exit)
    { // store a result
        std::unique_lock<co_mutex> _Lock(_Mtx);
        _Set_value_raw(std::forward<_Ty>(_Val), &_Lock, _At_thread_exit);
    }

    void _Set_value_raw(_Ty&& _Val, std::unique_lock<co_mutex>* _Lock,
                        bool _At_thread_exit)
    { // store a result while inside a locked block
        if (_Has_stored_result)
        {
            throw std::future_error(std::future_errc::promise_already_satisfied);
        }

        _Result = std::forward<_Ty>(_Val);
        _Do_notify(_Lock, _At_thread_exit);
    }

    void _Set_value(bool _At_thread_exit)
    { // store a (void) result
        std::unique_lock<co_mutex> _Lock(_Mtx);
        _Set_value_raw(&_Lock, _At_thread_exit);
    }

    void _Set_value_raw(
        std::unique_lock<co_mutex>* _Lock, bool _At_thread_exit)
    { // store a (void) result while inside a locked block
        if (_Has_stored_result)
        {
            throw std ::future_error(std::future_errc::promise_already_satisfied);
        }

        _Do_notify(_Lock, _At_thread_exit);
    }

    void _Set_exception(std::exception_ptr _Exc, bool _At_thread_exit)
    { // store a result
        std::unique_lock<co_mutex> _Lock(_Mtx);
        _Set_exception_raw(_Exc, &_Lock, _At_thread_exit);
    }

    void _Set_exception_raw(std::exception_ptr _Exc, std::unique_lock<co_mutex>* _Lock,
                            bool _At_thread_exit)
    { // store a result while inside a locked block
        if (_Has_stored_result)
        {
            throw std::future_error(std::future_errc::promise_already_satisfied);
        }

        _Exception = _Exc;
        _Do_notify(_Lock, _At_thread_exit);
    }

    bool _Is_ready() const
    {
        return _Ready != 0;
    }

    bool _Is_ready_at_thread_exit() const
    {
        return _Ready_at_thread_exit;
    }

    bool _Already_has_stored_result() const
    {
        return _Has_stored_result;
    }

    bool _Already_retrieved() const
    {
        return _Retrieved;
    }

    void _Abandon()
    { // abandon shared state
        std::unique_lock<co_mutex> _Lock(_Mtx);
        if (!_Has_stored_result)
        { // queue exception
            std::future_error _Fut(std::future_errc::broken_promise);
            _Set_exception_raw(std::make_exception_ptr(_Fut), &_Lock, false);
        }
    }

protected:
    void _Make_ready_at_thread_exit()
    { // set ready status at thread exit
        if (_Ready_at_thread_exit)
        {
            _Ready = true;
        }
    }

    void _Maybe_run_deferred_function(std::unique_lock<co_mutex>& _Lock)
    { // run a deferred function if not already done
        if (!_Running)
        { // run the function
            _Running = true;
            _Run_deferred_function(_Lock);
        }
    }

public:
    _Ty                   _Result;
    std::exception_ptr    _Exception;
    co_mutex              _Mtx;
    co_condition_variable _Cond;
    bool                  _Retrieved;
    int                   _Ready;
    bool                  _Ready_at_thread_exit;
    bool                  _Has_stored_result;
    bool                  _Running;

private:
    virtual bool _Has_deferred_function() const noexcept
    { // overridden by _Deferred_async_state
        return false;
    }

    virtual void _Run_deferred_function(std::unique_lock<co_mutex>&) {} // do nothing

    virtual void _Do_notify(std::unique_lock<co_mutex>* _Lock, bool _At_thread_exit)
    { // notify waiting threads
        // TRANSITION, ABI: This is virtual, but never overridden.
        _Has_stored_result = true;
        if (_At_thread_exit)
        { // notify at thread exit
          // _Cond._Register(*_Lock, &_Ready);
        }
        else
        { // notify immediately
            _Ready = true;
            _Cond.notify_all();
        }
    }

    void _Delete_this()
    { // delete this object
        if (_Deleter)
        {
            _Deleter->_Delete(this);
        }
        else
        {
            delete this;
        }
    }

    _Mydel* _Deleter;

public:
    _Associated_state(const _Associated_state&) = delete;
    _Associated_state& operator=(const _Associated_state&) = delete;
};

template <class>
class _Packaged_state;

template <class _Ret, class... _ArgTypes>
class _Packaged_state<_Ret(_ArgTypes...)>
    : public _Associated_state<_Ret>
{ // class for managing associated asynchronous state for packaged_task
public:
    using _Mybase = _Associated_state<_Ret>;
    using _Mydel  = typename _Mybase::_Mydel;

    template <class _Fty2>
    _Packaged_state(const _Fty2& _Fnarg)
        : _Fn(_Fnarg)
    {
    }

    template <class _Fty2>
    _Packaged_state(_Fty2&& _Fnarg)
        : _Fn(std::forward<_Fty2>(_Fnarg))
    {
    }

    void _Call_deferred(_ArgTypes... _Args)
    { // set deferred call
        try
        {
            // call function object and catch exceptions
            this->_Set_value(_Fn(std::forward<_ArgTypes>(_Args)...), true);
        }
        catch (...)
        {
            // function object threw exception; record result
            this->_Set_exception(std::current_exception(), true);
        }
    }

    void _Call_immediate(_ArgTypes... _Args)
    { // call function object
        try
        {
            // call function object and catch exceptions
            this->_Set_value(_Fn(std::forward<_ArgTypes>(_Args)...), false);
        }
        catch (...)
        {
            // function object threw exception; record result
            this->_Set_exception(std::current_exception(), false);
        }
    }

    const std::function<_Ret(_ArgTypes...)>& _Get_fn()
    {
        return _Fn;
    }

private:
    std::function<_Ret(_ArgTypes...)> _Fn;
};

template <class _Ret, class... _ArgTypes>
class _Packaged_state<_Ret&(_ArgTypes...)>
    : public _Associated_state<_Ret*>
{ // class for managing associated asynchronous state for packaged_task
public:
    using _Mybase = _Associated_state<_Ret*>;
    using _Mydel  = typename _Mybase::_Mydel;

    template <class _Fty2>
    _Packaged_state(const _Fty2& _Fnarg)
        : _Fn(_Fnarg)
    {
    }

    template <class _Fty2>
    _Packaged_state(_Fty2&& _Fnarg)
        : _Fn(std::forward<_Fty2>(_Fnarg))
    {
    }

    void _Call_deferred(_ArgTypes... _Args)
    { // set deferred call
        try
        {
            // call function object and catch exceptions
            this->_Set_value(std::addressof(_Fn(std::forward<_ArgTypes>(_Args)...)), true);
        }
        catch (...)
        {
            // function object threw exception; record result
            this->_Set_exception(std::current_exception(), true);
        }
    }

    void _Call_immediate(_ArgTypes... _Args)
    { // call function object
        try
        {
            // call function object and catch exceptions
            this->_Set_value(std::addressof(_Fn(std::forward<_ArgTypes>(_Args)...)), false);
        }
        catch (...)
        {
            // function object threw exception; record result
            this->_Set_exception(std::current_exception(), false);
        }
    }

    const std::function<_Ret&(_ArgTypes...)>& _Get_fn()
    {
        return _Fn;
    }

private:
    std::function<_Ret&(_ArgTypes...)> _Fn;
};

template <class... _ArgTypes>
class _Packaged_state<void(_ArgTypes...)>
    : public _Associated_state<int>
{ // class for managing associated asynchronous state for packaged_task
public:
    using _Mybase = _Associated_state<int>;
    using _Mydel  = typename _Mybase::_Mydel;

    template <class _Fty2>
    _Packaged_state(const _Fty2& _Fnarg)
        : _Fn(_Fnarg)
    {
    }

    template <class _Fty2>
    _Packaged_state(_Fty2&& _Fnarg)
        : _Fn(std::forward<_Fty2>(_Fnarg))
    {
    }

    void _Call_deferred(_ArgTypes... _Args)
    { // set deferred call
        try
        {
            // call function object and catch exceptions
            _Fn(std::forward<_ArgTypes>(_Args)...);
            this->_Set_value(1, true);
        }
        catch (...)
        {
            // function object threw exception; record result
            this->_Set_exception(std::current_exception(), true);
        }
    }

    void _Call_immediate(_ArgTypes... _Args)
    { // call function object
        try
        {
            // call function object and catch exceptions
            _Fn(std::forward<_ArgTypes>(_Args)...);
            this->_Set_value(1, false);
        }
        catch (...)
        {
            // function object threw exception; record result
            this->_Set_exception(std::current_exception(), false);
        }
    }

    const std::function<void(_ArgTypes...)>& _Get_fn()
    {
        return _Fn;
    }

private:
    std::function<void(_ArgTypes...)> _Fn;
};

template <class _Rx>
class _Deferred_async_state : public _Packaged_state<_Rx()>
{
    // class for managing associated synchronous state for deferred execution from async
public:
    template <class _Fty2>
    _Deferred_async_state(const _Fty2& _Fnarg)
        : _Packaged_state<_Rx()>(_Fnarg)
    {
    }

    template <class _Fty2>
    _Deferred_async_state(_Fty2&& _Fnarg)
        : _Packaged_state<_Rx()>(std::forward<_Fty2>(_Fnarg))
    {
    }

private:
    bool _Has_deferred_function() const noexcept override
    {
        // this function is considered to be deferred until it's invoked
        return !this->_Running;
    }

    void _Run_deferred_function(std::unique_lock<co_mutex>& _Lock) override
    { // run the deferred function
        _Lock.unlock();
        _Packaged_state<_Rx()>::_Call_immediate();
        _Lock.lock();
    }
};

template <class _Rx>
class _Task_async_state : public _Packaged_state<_Rx()>
{
    // class for managing associated synchronous state for asynchronous execution from async
public:
    using _Mybase     = _Packaged_state<_Rx()>;
    using _State_type = typename _Mybase::_State_type;

    template <class _Fty2>
    _Task_async_state(_Fty2&& _Fnarg)
        : _Mybase(std::forward<_Fty2>(_Fnarg))
    {
        // _Task = ::Concurrency::create_task([this]() { // do it now
        //     this->_Call_immediate();
        // });
        co({}, [this] { this->_Call_immediate(); }).detach();
        this->_Running = true;
    }

    ~_Task_async_state() noexcept override
    {
        _Wait();
    }

    void _Wait() override
    { // wait for completion
        _Task->join();
    }

    _State_type& _Get_value(bool _Get_only_once) override
    {
        // return the stored result or throw stored exception
        _Task->join();
        return _Mybase::_Get_value(_Get_only_once);
    }

private:
    std::shared_ptr<co> _Task;
};

template <class _Ty>
class _State_manager
{
    // class for managing possibly non-existent associated asynchronous state object
public:
    _State_manager()
        : _Assoc_state(nullptr)
    { // construct with no associated asynchronous state object
        _Get_only_once = false;
    }

    _State_manager(_Associated_state<_Ty>* _New_state, bool _Get_once)
        : _Assoc_state(_New_state)
    { // construct with _New_state
        _Get_only_once = _Get_once;
    }

    _State_manager(const _State_manager& _Other, bool _Get_once = false)
        : _Assoc_state(nullptr)
    {
        _Copy_from(_Other);
        _Get_only_once = _Get_once;
    }

    _State_manager(_State_manager&& _Other, bool _Get_once = false)
        : _Assoc_state(nullptr)
    {
        _Move_from(_Other);
        _Get_only_once = _Get_once;
    }

    ~_State_manager() noexcept
    {
        if (_Assoc_state)
        {
            _Assoc_state->_Release();
        }
    }

    _State_manager& operator=(const _State_manager& _Other)
    {
        _Copy_from(_Other);
        return *this;
    }

    _State_manager& operator=(_State_manager&& _Other)
    {
        _Move_from(_Other);
        return *this;
    }

    [[nodiscard]] bool valid() const noexcept
    {
        return _Assoc_state && !(_Get_only_once && _Assoc_state->_Already_retrieved());
    }

    void wait() const
    { // wait for signal
        if (!valid())
        {
            throw std::future_error(std::future_errc::no_state);
        }

        _Assoc_state->_Wait();
    }

    template <class _Rep, class _Per>
    std::future_status wait_for(const std::chrono::duration<_Rep, _Per>& _Rel_time) const
    { // wait for duration
        if (!valid())
        {
            throw std::future_error(std::future_errc::no_state);
        }

        return _Assoc_state->_Wait_for(_Rel_time);
    }

    template <class _Clock, class _Dur>
    std::future_status wait_until(const std::chrono::time_point<_Clock, _Dur>& _Abs_time) const
    { // wait until time point
#if _HAS_CXX20
        static_assert(chrono::is_clock_v<_Clock>, "Clock type required");
#endif // _HAS_CXX20
        if (!valid())
        {
            throw std::future_error(std::future_errc::no_state);
        }

        return _Assoc_state->_Wait_until(_Abs_time);
    }

    _Ty& _Get_value() const
    {
        if (!valid())
        {
            throw std::future_error(std::future_errc::no_state);
        }

        return _Assoc_state->_Get_value(_Get_only_once);
    }

    void _Set_value(const _Ty& _Val, bool _Defer)
    { // store a result
        if (!valid())
        {
            throw std::future_error(std::future_errc::no_state);
        }

        _Assoc_state->_Set_value(_Val, _Defer);
    }

    void _Set_value(_Ty&& _Val, bool _Defer)
    { // store a result
        if (!valid())
        {
            throw std::future_error(std::future_errc::no_state);
        }

        _Assoc_state->_Set_value(std::forward<_Ty>(_Val), _Defer);
    }

    void _Abandon()
    { // abandon shared state
        if (_Assoc_state)
        {
            _Assoc_state->_Abandon();
        }
    }

    void _Set_exception(std::exception_ptr _Exc, bool _Defer)
    { // store a result
        if (!valid())
        {
            throw std::future_error(std::future_errc::no_state);
        }

        _Assoc_state->_Set_exception(_Exc, _Defer);
    }

    void _Swap(_State_manager& _Other)
    { // exchange with _Other
        std::swap(_Assoc_state, _Other._Assoc_state);
    }

    _Associated_state<_Ty>* _Ptr() const
    {
        return _Assoc_state;
    }

    void _Copy_from(const _State_manager& _Other)
    { // copy stored associated asynchronous state object from _Other
        if (this != std::addressof(_Other))
        {
            if (_Assoc_state)
            {
                _Assoc_state->_Release();
            }

            if (_Other._Assoc_state)
            { // do the copy
                _Other._Assoc_state->_Retain();
                _Assoc_state   = _Other._Assoc_state;
                _Get_only_once = _Other._Get_only_once;
            }
            else
            {
                _Assoc_state = nullptr;
            }
        }
    }

    void _Move_from(_State_manager& _Other)
    { // move stored associated asynchronous state object from _Other
        if (this != std::addressof(_Other))
        {
            if (_Assoc_state)
            {
                _Assoc_state->_Release();
            }

            _Assoc_state        = _Other._Assoc_state;
            _Other._Assoc_state = nullptr;
            _Get_only_once      = _Other._Get_only_once;
        }
    }

    bool _Is_ready() const
    {
        return _Assoc_state && _Assoc_state->_Is_ready();
    }

    bool _Is_ready_at_thread_exit() const
    {
        return _Assoc_state && _Assoc_state->_Is_ready_at_thread_exit();
    }

private:
    _Associated_state<_Ty>* _Assoc_state;
    bool                    _Get_only_once;
};

template <class _Ty>
class shared_future;

template <class _Ty>
class future : public _State_manager<_Ty>
{
    // class that defines a non-copyable asynchronous return object that holds a value
    using _Mybase = _State_manager<_Ty>;

public:
    static_assert(!std::is_array_v<_Ty> && std::is_object_v<_Ty> && std::is_destructible_v<_Ty>,
                  "T in future<T> must meet the Cpp17Destructible requirements (N4878 [futures.unique.future]/4).");

    future() noexcept {}

    future(future&& _Other) noexcept
        : _Mybase(std::move(_Other), true)
    {
    }

    future& operator=(future&& _Right) noexcept
    {
        _Mybase::operator=(std::move(_Right));
        return *this;
    }

    future(const _Mybase& _State, _Nil)
        : _Mybase(_State, true)
    {
    }

    ~future() noexcept {}

    _Ty get()
    {
        // block until ready then return the stored result or throw the stored exception
        future _Local { std::move(*this) };
        return std::move(_Local._Get_value());
    }

    [[nodiscard]] shared_future<_Ty> share() noexcept
    {
        return shared_future<_Ty>(std::move(*this));
    }

    future(const future&) = delete;
    future& operator=(const future&) = delete;
};

template <class _Ty>
class future<_Ty&> : public _State_manager<_Ty*>
{
    // class that defines a non-copyable asynchronous return object that holds a reference
    using _Mybase = _State_manager<_Ty*>;

public:
    future() noexcept {}

    future(future&& _Other) noexcept
        : _Mybase(std::move(_Other), true)
    {
    }

    future& operator=(future&& _Right) noexcept
    {
        _Mybase::operator=(std::move(_Right));
        return *this;
    }

    future(const _Mybase& _State, _Nil)
        : _Mybase(_State, true)
    {
    }

    ~future() noexcept {}

    _Ty& get()
    {
        // block until ready then return the stored result or throw the stored exception
        future _Local { std::move(*this) };
        return *_Local._Get_value();
    }

    [[nodiscard]] shared_future<_Ty&> share() noexcept
    {
        return shared_future<_Ty&>(std::move(*this));
    }

    future(const future&) = delete;
    future& operator=(const future&) = delete;
};

template <>
class future<void> : public _State_manager<int>
{
    // class that defines a non-copyable asynchronous return object that does not hold a value
    using _Mybase = _State_manager<int>;

public:
    future() noexcept {}

    future(future&& _Other) noexcept
        : _Mybase(std::move(_Other), true)
    {
    }

    future& operator=(future&& _Right) noexcept
    {
        _Mybase::operator=(std::move(_Right));
        return *this;
    }

    future(const _Mybase& _State, _Nil)
        : _Mybase(_State, true)
    {
    }

    ~future() noexcept {}

    void get()
    {
        // block until ready then return or throw the stored exception
        future _Local { std::move(*this) };
        _Local._Get_value();
    }

    [[nodiscard]] shared_future<void> share() noexcept;

    future(const future&) = delete;
    future& operator=(const future&) = delete;
};

template <class _Ty>
class shared_future : public _State_manager<_Ty>
{
    // class that defines a copyable asynchronous return object that holds a value
    using _Mybase = _State_manager<_Ty>;

public:
    static_assert(!std::is_array_v<_Ty> && std::is_object_v<_Ty> && std::is_destructible_v<_Ty>,
                  "T in shared_future<T> must meet the Cpp17Destructible requirements (N4878 [futures.shared.future]/4).");

    shared_future() noexcept {}

    shared_future(const shared_future& _Other) noexcept
        : _Mybase(_Other)
    {
    }

    shared_future& operator=(const shared_future& _Right) noexcept
    {
        _Mybase::operator=(_Right);
        return *this;
    }

    shared_future(future<_Ty>&& _Other) noexcept
        : _Mybase(std::forward<_Mybase>(_Other))
    {
    }

    shared_future(shared_future&& _Other) noexcept
        : _Mybase(std::move(_Other))
    {
    }

    shared_future& operator=(shared_future&& _Right) noexcept
    {
        _Mybase::operator=(std::move(_Right));
        return *this;
    }

    ~shared_future() noexcept {}

    const _Ty& get() const
    {
        // block until ready then return the stored result or throw the stored exception
        return this->_Get_value();
    }
};

template <class _Ty>
class shared_future<_Ty&> : public _State_manager<_Ty*>
{
    // class that defines a copyable asynchronous return object that holds a reference
    using _Mybase = _State_manager<_Ty*>;

public:
    shared_future() noexcept {}

    shared_future(const shared_future& _Other) noexcept
        : _Mybase(_Other)
    {
    }

    shared_future& operator=(const shared_future& _Right) noexcept
    {
        _Mybase::operator=(_Right);
        return *this;
    }

    shared_future(future<_Ty&>&& _Other) noexcept
        : _Mybase(std::forward<_Mybase>(_Other))
    {
    }

    shared_future(shared_future&& _Other) noexcept
        : _Mybase(std::move(_Other))
    {
    }

    shared_future& operator=(shared_future&& _Right) noexcept
    {
        _Mybase::operator=(std::move(_Right));
        return *this;
    }

    ~shared_future() noexcept {}

    _Ty& get() const
    {
        // block until ready then return the stored result or throw the stored exception
        return *this->_Get_value();
    }
};

template <>
class shared_future<void> : public _State_manager<int>
{
    // class that defines a copyable asynchronous return object that does not hold a value
    using _Mybase = _State_manager<int>;

public:
    shared_future() noexcept {}

    shared_future(const shared_future& _Other) noexcept
        : _Mybase(_Other)
    {
    }

    shared_future& operator=(const shared_future& _Right) noexcept
    {
        _Mybase::operator=(_Right);
        return *this;
    }

    shared_future(shared_future&& _Other) noexcept
        : _Mybase(std::move(_Other))
    {
    }

    shared_future(future<void>&& _Other) noexcept
        : _Mybase(std::forward<_Mybase>(_Other))
    {
    }

    shared_future& operator=(shared_future&& _Right)
    {
        _Mybase::operator=(std::move(_Right));
        return *this;
    }

    ~shared_future() noexcept {}

    void get() const
    { // block until ready then return or throw the stored exception
        this->_Get_value();
    }
};

[[nodiscard]] inline shared_future<void> future<void>::share() noexcept
{
    return shared_future<void>(std::move(*this));
}

template <class _Ty>
class _Promise
{
public:
    _Promise(_Associated_state<_Ty>* _State_ptr)
        : _State(_State_ptr, false)
        , _Future_retrieved(false)
    {
    }

    _Promise(_Promise&& _Other)
        : _State(std::move(_Other._State))
        , _Future_retrieved(_Other._Future_retrieved)
    {
    }

    _Promise& operator=(_Promise&& _Other)
    {
        _State            = std::move(_Other._State);
        _Future_retrieved = _Other._Future_retrieved;
        return *this;
    }

    ~_Promise() noexcept {}

    void _Swap(_Promise& _Other)
    {
        _State._Swap(_Other._State);
        std::swap(_Future_retrieved, _Other._Future_retrieved);
    }

    const _State_manager<_Ty>& _Get_state() const
    {
        return _State;
    }
    _State_manager<_Ty>& _Get_state()
    {
        return _State;
    }

    _State_manager<_Ty>& _Get_state_for_set()
    {
        if (!_State.valid())
        {
            throw std::future_error(std::future_errc::no_state);
        }

        return _State;
    }

    _State_manager<_Ty>& _Get_state_for_future()
    {
        if (!_State.valid())
        {
            throw std::future_error(std::future_errc::no_state);
        }

        if (_Future_retrieved)
        {
            throw std::future_error(std::future_errc::future_already_retrieved);
        }

        _Future_retrieved = true;
        return _State;
    }

    bool _Is_valid() const noexcept
    {
        return _State.valid();
    }

    bool _Is_ready() const
    {
        return _State._Is_ready();
    }

    bool _Is_ready_at_thread_exit() const
    {
        return _State._Is_ready_at_thread_exit();
    }

    _Promise(const _Promise&) = delete;
    _Promise& operator=(const _Promise&) = delete;

private:
    _State_manager<_Ty> _State;
    bool                _Future_retrieved;
};

template <class _Ty>
class promise
{ // class that defines an asynchronous provider that holds a value
public:
    static_assert(!std::is_array_v<_Ty> && std::is_object_v<_Ty> && std::is_destructible_v<_Ty>,
                  "T in promise<T> must meet the Cpp17Destructible requirements (N4878 [futures.promise]/1).");

    promise()
        : _MyPromise(new _Associated_state<_Ty>)
    {
    }

    template <class _Alloc>
    promise(std::allocator_arg_t, const _Alloc& _Al)
        : _MyPromise(_Make_associated_state<_Ty>(_Al))
    {
    }

    promise(promise&& _Other) noexcept
        : _MyPromise(std::move(_Other._MyPromise))
    {
    }

    promise& operator=(promise&& _Other) noexcept
    {
        promise(std::move(_Other)).swap(*this);
        return *this;
    }

    ~promise() noexcept
    {
        if (_MyPromise._Is_valid() && !_MyPromise._Is_ready() && !_MyPromise._Is_ready_at_thread_exit())
        {
            // exception if destroyed before function object returns
            std::future_error _Fut(std::future_errc::broken_promise);
            _MyPromise._Get_state()._Set_exception(std::make_exception_ptr(_Fut), false);
        }
    }

    void swap(promise& _Other) noexcept
    {
        _MyPromise._Swap(_Other._MyPromise);
    }

    [[nodiscard]] future<_Ty> get_future()
    {
        return future<_Ty>(_MyPromise._Get_state_for_future(), _Nil {});
    }

    void set_value(const _Ty& _Val)
    {
        _MyPromise._Get_state_for_set()._Set_value(_Val, false);
    }

    void set_value_at_thread_exit(const _Ty& _Val)
    {
        _MyPromise._Get_state_for_set()._Set_value(_Val, true);
    }

    void set_value(_Ty&& _Val)
    {
        _MyPromise._Get_state_for_set()._Set_value(std::forward<_Ty>(_Val), false);
    }

    void set_value_at_thread_exit(_Ty&& _Val)
    {
        _MyPromise._Get_state_for_set()._Set_value(std::forward<_Ty>(_Val), true);
    }

    void set_exception(std::exception_ptr _Exc)
    {
        _MyPromise._Get_state_for_set()._Set_exception(_Exc, false);
    }

    void set_exception_at_thread_exit(std::exception_ptr _Exc)
    {
        _MyPromise._Get_state_for_set()._Set_exception(_Exc, true);
    }

    promise(const promise&) = delete;
    promise& operator=(const promise&) = delete;

private:
    _Promise<_Ty> _MyPromise;
};

template <class _Ty>
class promise<_Ty&>
{ // class that defines an asynchronous provider that holds a reference
public:
    promise()
        : _MyPromise(new _Associated_state<_Ty*>)
    {
    }

    template <class _Alloc>
    promise(std::allocator_arg_t, const _Alloc& _Al)
        : _MyPromise(_Make_associated_state<_Ty*>(_Al))
    {
    }

    promise(promise&& _Other) noexcept
        : _MyPromise(std::move(_Other._MyPromise))
    {
    }

    promise& operator=(promise&& _Other) noexcept
    {
        promise(std::move(_Other)).swap(*this);
        return *this;
    }

    ~promise() noexcept
    {
        if (_MyPromise._Is_valid() && !_MyPromise._Is_ready() && !_MyPromise._Is_ready_at_thread_exit())
        {
            // exception if destroyed before function object returns
            std::future_error _Fut(std::future_errc::broken_promise);
            _MyPromise._Get_state()._Set_exception(std::make_exception_ptr(_Fut), false);
        }
    }

    void swap(promise& _Other) noexcept
    {
        _MyPromise._Swap(_Other._MyPromise);
    }

    [[nodiscard]] future<_Ty&> get_future()
    {
        return future<_Ty&>(_MyPromise._Get_state_for_future(), _Nil {});
    }

    void set_value(_Ty& _Val)
    {
        _MyPromise._Get_state_for_set()._Set_value(std::addressof(_Val), false);
    }

    void set_value_at_thread_exit(_Ty& _Val)
    {
        _MyPromise._Get_state_for_set()._Set_value(std::addressof(_Val), true);
    }

    void set_exception(std::exception_ptr _Exc)
    {
        _MyPromise._Get_state_for_set()._Set_exception(_Exc, false);
    }

    void set_exception_at_thread_exit(std::exception_ptr _Exc)
    {
        _MyPromise._Get_state_for_set()._Set_exception(_Exc, true);
    }

    promise(const promise&) = delete;
    promise& operator=(const promise&) = delete;

private:
    _Promise<_Ty*> _MyPromise;
};

template <>
class promise<void>
{ // defines an asynchronous provider that does not hold a value
public:
    promise()
        : _MyPromise(new _Associated_state<int>)
    {
    }

    template <class _Alloc>
    promise(std::allocator_arg_t, const _Alloc& _Al)
        : _MyPromise(_Make_associated_state<int>(_Al))
    {
    }

    promise(promise&& _Other) noexcept
        : _MyPromise(std::move(_Other._MyPromise))
    {
    }

    promise& operator=(promise&& _Other) noexcept
    {
        promise(std::move(_Other)).swap(*this);
        return *this;
    }

    ~promise() noexcept
    {
        if (_MyPromise._Is_valid() && !_MyPromise._Is_ready() && !_MyPromise._Is_ready_at_thread_exit())
        {
            // exception if destroyed before function object returns
            std::future_error _Fut(std::future_errc::broken_promise);
            _MyPromise._Get_state()._Set_exception(std::make_exception_ptr(_Fut), false);
        }
    }

    void swap(promise& _Other) noexcept
    {
        _MyPromise._Swap(_Other._MyPromise);
    }

    [[nodiscard]] future<void> get_future()
    {
        return future<void>(_MyPromise._Get_state_for_future(), _Nil {});
    }

    void set_value()
    {
        _MyPromise._Get_state_for_set()._Set_value(1, false);
    }

    void set_value_at_thread_exit()
    {
        _MyPromise._Get_state_for_set()._Set_value(1, true);
    }

    void set_exception(std::exception_ptr _Exc)
    {
        _MyPromise._Get_state_for_set()._Set_exception(_Exc, false);
    }

    void set_exception_at_thread_exit(std::exception_ptr _Exc)
    {
        _MyPromise._Get_state_for_set()._Set_exception(_Exc, true);
    }

    promise(const promise&) = delete;
    promise& operator=(const promise&) = delete;

private:
    _Promise<int> _MyPromise;
};

template <class _Ty>
void swap(promise<_Ty>& _Left, promise<_Ty>& _Right) noexcept
{
    _Left.swap(_Right);
}

template <class _Fret>
struct _P_arg_type
{ // type for functions returning T
    using type = _Fret;
};

template <class _Fret>
struct _P_arg_type<_Fret&>
{ // type for functions returning reference to T
    using type = _Fret*;
};

template <>
struct _P_arg_type<void>
{ // type for functions returning void
    using type = int;
};

template <class>
class packaged_task; // not defined

template <class _Ret, class... _ArgTypes>
class packaged_task<_Ret(_ArgTypes...)>
{
    // class that defines an asynchronous provider that returns the result of a call to a function object
public:
    using _Ptype              = typename _P_arg_type<_Ret>::type;
    using _MyPromiseType      = _Promise<_Ptype>;
    using _MyStateManagerType = _State_manager<_Ptype>;
    using _MyStateType        = _Packaged_state<_Ret(_ArgTypes...)>;

    packaged_task() noexcept
        : _MyPromise(0)
    {
    }

    template <class _Fty2, std::enable_if_t<!std::is_same_v<std::decay_t<_Fty2>, packaged_task>, int> = 0>
    explicit packaged_task(_Fty2&& _Fnarg)
        : _MyPromise(new _MyStateType(std::forward<_Fty2>(_Fnarg)))
    {
    }

    packaged_task(packaged_task&& _Other) noexcept
        : _MyPromise(std::move(_Other._MyPromise))
    {
    }

    packaged_task& operator=(packaged_task&& _Other) noexcept
    {
        _MyPromise = std::move(_Other._MyPromise);
        return *this;
    }

    ~packaged_task() noexcept
    {
        _MyPromise._Get_state()._Abandon();
    }

    void swap(packaged_task& _Other) noexcept
    {
        std::swap(_MyPromise, _Other._MyPromise);
    }

    [[nodiscard]] bool valid() const noexcept
    {
        return _MyPromise._Is_valid();
    }

    [[nodiscard]] future<_Ret> get_future()
    {
        return future<_Ret>(_MyPromise._Get_state_for_future(), _Nil {});
    }

    void operator()(_ArgTypes... _Args)
    {
        if (_MyPromise._Is_ready())
        {
            throw std::future_error(std::future_errc::promise_already_satisfied);
        }

        _MyStateManagerType& _State = _MyPromise._Get_state_for_set();
        _MyStateType*        _Ptr   = static_cast<_MyStateType*>(_State._Ptr());
        _Ptr->_Call_immediate(std::forward<_ArgTypes>(_Args)...);
    }

    void make_ready_at_thread_exit(_ArgTypes... _Args)
    {
        if (_MyPromise._Is_ready())
        {
            throw std::future_error(std::future_errc::promise_already_satisfied);
        }

        _MyStateManagerType& _State = _MyPromise._Get_state_for_set();
        if (_State._Ptr()->_Already_has_stored_result())
        {
            throw std::future_error(std::future_errc::promise_already_satisfied);
        }

        _MyStateType* _Ptr = static_cast<_MyStateType*>(_State._Ptr());
        _Ptr->_Call_deferred(std::forward<_ArgTypes>(_Args)...);
    }

    void reset()
    { // reset to newly constructed state
        _MyStateManagerType&              _State   = _MyPromise._Get_state_for_set();
        _MyStateType*                     _MyState = static_cast<_MyStateType*>(_State._Ptr());
        std::function<_Ret(_ArgTypes...)> _Fnarg   = _MyState->_Get_fn();
        _MyPromiseType                    _New_promise(new _MyStateType(_Fnarg));
        _MyPromise._Get_state()._Abandon();
        _MyPromise._Swap(_New_promise);
    }

    packaged_task(const packaged_task&) = delete;
    packaged_task& operator=(const packaged_task&) = delete;

private:
    _MyPromiseType _MyPromise;
};

#if _HAS_CXX17
#define _PACKAGED_TASK_DEDUCTION_GUIDE(CALL_OPT, X1, X2, X3) \
    template <class _Ret, class... _Types>                   \
    packaged_task(_Ret(CALL_OPT*)(_Types...)) -> packaged_task<_Ret(_Types...)>; // intentionally discards CALL_OPT

_NON_MEMBER_CALL(_PACKAGED_TASK_DEDUCTION_GUIDE, X1, X2, X3)
#undef _PACKAGED_TASK_DEDUCTION_GUIDE

template <class _Fx>
packaged_task(_Fx) -> packaged_task<typename _Deduce_signature<_Fx>::type>;
#endif // _HAS_CXX17

#if _HAS_FUNCTION_ALLOCATOR_SUPPORT
template <class _Ty, class _Alloc>
struct uses_allocator<packaged_task<_Ty>, _Alloc> : true_type
{
};
#endif // _HAS_FUNCTION_ALLOCATOR_SUPPORT

template <class _Ty>
void swap(packaged_task<_Ty>& _Left, packaged_task<_Ty>& _Right) noexcept
{
    _Left.swap(_Right);
}

template <class... _Types, size_t... _Indices>
auto _Invoke_stored_explicit(std::tuple<_Types...>&& _Tuple, std::index_sequence<_Indices...>) -> decltype(std::invoke(
    std::get<_Indices>(std::move(_Tuple))...))
{ // invoke() a tuple with explicit parameter ordering
    return std::invoke(std::get<_Indices>(std::move(_Tuple))...);
}

template <class... _Types>
auto _Invoke_stored(std::tuple<_Types...>&& _Tuple)
    -> decltype(_Invoke_stored_explicit(std::move(_Tuple), std::index_sequence_for<_Types...> {}))
{ // invoke() a tuple
    return _Invoke_stored_explicit(std::move(_Tuple), std::index_sequence_for<_Types...> {});
}

template <class... _Types>
class _Fake_no_copy_callable_adapter
{
    // async() is built on packaged_task internals which incorrectly use
    // std::function, which requires that things be copyable. We can't fix this in an
    // update, so this adapter turns copies into terminate(). When VSO-153581 is
    // fixed, remove this adapter.
    using _Storaget = std::tuple<std::decay_t<_Types>...>;

public:
    explicit _Fake_no_copy_callable_adapter(_Types&&... _Vals)
        : _Storage(std::forward<_Types>(_Vals)...)
    {
        // Initializes _Fake_no_copy_callable_adapter with a decayed callable object and arguments
    }

    _Fake_no_copy_callable_adapter(const _Fake_no_copy_callable_adapter& _Other)
        : _Storage(std::move(_Other._Storage))
    {
        std::terminate(); // Very Bad Things
    }

    _Fake_no_copy_callable_adapter(_Fake_no_copy_callable_adapter&& _Other) = default;
    _Fake_no_copy_callable_adapter& operator=(const _Fake_no_copy_callable_adapter&) = delete;
    _Fake_no_copy_callable_adapter& operator=(_Fake_no_copy_callable_adapter&&) = delete;

    auto operator()() -> decltype(_Invoke_stored(std::move(std::declval<_Storaget&>())))
    {
        return _Invoke_stored(std::move(_Storage));
    }

private:
    mutable _Storaget _Storage;
};

template <class _Ret, class _Fty>
_Associated_state<typename _P_arg_type<_Ret>::type>* _Get_associated_state(
    std::launch _Psync, _Fty&& _Fnarg)
{ // construct associated asynchronous state object for the launch type
    switch (_Psync)
    { // select launch type
    case std::launch::deferred:
        return new _Deferred_async_state<_Ret>(std::forward<_Fty>(_Fnarg));
    case std::launch::async: // TRANSITION, fixed in vMajorNext, should create a new thread here
    default:
        return new _Task_async_state<_Ret>(std::forward<_Fty>(_Fnarg));
    }
}

template <class _Fty, class... _ArgTypes>
[[nodiscard]] future<std::invoke_result_t<std::decay_t<_Fty>, std::decay_t<_ArgTypes>...>> async(
    std::launch _Policy, _Fty&& _Fnarg, _ArgTypes&&... _Args)
{
    // manages a callable object launched with supplied policy
    using _Ret   = std::invoke_result_t<std::decay_t<_Fty>, std::decay_t<_ArgTypes>...>;
    using _Ptype = typename _P_arg_type<_Ret>::type;
    _Promise<_Ptype> _Pr(
        _Get_associated_state<_Ret>(_Policy, _Fake_no_copy_callable_adapter<_Fty, _ArgTypes...>(
                                                 std::forward<_Fty>(_Fnarg), std::forward<_ArgTypes>(_Args)...)));

    return future<_Ret>(_Pr._Get_state_for_future(), _Nil {});
}

template <class _Fty, class... _ArgTypes>
[[nodiscard]] future<std::invoke_result_t<std::decay_t<_Fty>, std::decay_t<_ArgTypes>...>> async(_Fty&& _Fnarg, _ArgTypes&&... _Args)
{
    // manages a callable object launched with default policy
    return std::async(std::launch::async | std::launch::deferred, std::forward<_Fty>(_Fnarg), std::forward<_ArgTypes>(_Args)...);
}

CO_NAMESPACE_END