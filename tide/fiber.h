#ifndef __TIDE_FIBER_H__
#define __TIDE_FIBER_H__

#include <memory>
#include <functional>

#include <ucontext.h>

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
        void call();
        void back();
        uint64_t getId() const {return m_id;}
        State getState() const { return m_state; }

    public:
        static void SetThis(Fiber* f);
        static Fiber::ptr GetThis();
        // 协程切换到后台，设置为ready状态
        static void YiedToReady();
        // 协程切换到前台，设置为hold状态
        static void YiedToHold();
        // 协程数量获取
        static uint64_t TotalFibers();

        static void MainFunc();
        static void CallerMainFunc();

        static uint64_t GetFiberId();

    private:
        uint64_t m_id = 0;
        uint32_t m_stacksize = 0;
        State m_state = INIT;

        ucontext_t m_ctx;
        void* m_stack = nullptr;

        std::function<void()> m_cb;
    };


}

#endif // __TIDE_FIBER_H__
