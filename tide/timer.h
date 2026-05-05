#ifndef __TIDE_TIMER_H__
#define __TIDE_TIMER_H__

#include <memory>
#include <functional>
#include <set>
#include <vector>

#include "thread.h"

namespace tide
{

    class TimerManager;
    class Timer : public std::enable_shared_from_this<Timer>
    {
    friend class TimerManager;
    public:
        using ptr = std::shared_ptr<Timer>;

    public:
        bool cancel();
        bool refresh();
        bool reset(uint64_t ms, bool from_now);

    private:
        Timer(uint64_t ms, std::function<void()> cb, bool recurring, TimerManager *manager);
        Timer(uint64_t next);

    

    private:
        bool m_recurring = false;          // 是否重复
        uint64_t m_ms = 0;                 // 定时器超时时间
        uint64_t m_next = 0;               // 下次执行的时间
        std::function<void()> m_cb;        // 定时器回调函数
        TimerManager *m_manager = nullptr; // 所属的定时器管理器

    private:
        struct Comparator
        {
            bool operator()(const Timer::ptr &lhs, const Timer::ptr &rhs) const;
        };
    };

    class TimerManager
    {
        friend class Timer;
    public:
        using RWMutexType = RWMutex;

        TimerManager();
        virtual ~TimerManager();

        Timer::ptr addTimer(uint64_t ms, std::function<void()> cb, bool recurring = false);
        Timer::ptr addConditionTimer(uint64_t ms, std::function<void()> cb, std::weak_ptr<void> weak_cond, bool recurring = false);

        uint64_t getNextTimer();    //下一个定时器的执行时间
        void listExpiredCb(std::vector<std::function<void()>> &cbs);
        bool hasTimer();

    protected:
        virtual void onTimerInsertedAtFront() = 0;
        void addTimer(Timer::ptr val, RWMutexType::WriteLock &lock);

    private:
        bool detectClockRollover(uint64_t now_ms);

    private:
        RWMutexType m_mutex;
        std::set<Timer::ptr, Timer::Comparator> m_timers;
        bool m_tickled = false;  // 是否已经tickle
        uint64_t m_previouseTime = 0; // 上次执行的时间
    };

};

#endif //__TIDE_TIMER_H__