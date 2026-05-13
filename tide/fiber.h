#ifndef TIDE_FIBER_H__
#define TIDE_FIBER_H__

#include <memory>
#include <functional>

#include <ucontext.h>
#include <stdint.h>

namespace tide
{
    class Scheduler;
    class Fiber : public std::enable_shared_from_this<Fiber>
    {
    friend class Scheduler;
    public:
        using ptr = std::shared_ptr<Fiber>;
        enum State{
            INIT,
            HOLD,
            EXEC,
            TERM,
            READY,
            EXCEPT
        };

    private:
        Fiber();

    public:
        Fiber(std::function<void()> cb, size_t stacksize = 0, bool use_caller = false);
        ~Fiber();

        // 重置协程函数并重置状态
        void reset(std::function<void()> cb);
        // 切换到当前协程执行
        void swapIn();
        // 切换到后台执行
        void swapOut();
        // 协程切换到当前协程执行
        void call();
        // 协程切换到后台执行
        void back();
        uint64_t getId() const {return m_id;}
        State getState() const { return m_state; }

    public:
        static void SetThis(Fiber* f);
        static Fiber::ptr GetThis();
        // 协程切换到后台，设置为ready状态
        static void YiedToReady();
        // 协程切换到前台，设置为hold状态
        static void YieldToHold();
        // 协程数量获取
        static uint64_t TotalFibers();

        static void MainFunc();
        static void CallerMainFunc();

        static uint64_t GetFiberId();

    private:
        uint64_t m_id = 0;  // 协程ID
        uint32_t m_stacksize = 0;   // 协程栈大小
        State m_state = INIT;   // 协程状态

        ucontext_t m_ctx;   // 协程上下文
        void* m_stack = nullptr;    // 协程栈指针

        std::function<void()> m_cb; // 协程执行函数
    };


}

#endif // TIDE_FIBER_H__
