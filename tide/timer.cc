#include "timer.h"

#include "utils.h"
#include "thread.h"
#include "log.h"

namespace tide
{

    static Logger::ptr g_timer_logger = TIDE_LOG_NAME("system");

    // 定时器比较器，按照定时器的下次执行时间进行排序
    bool Timer::Comparator::operator()(const Timer::ptr &lhs, const Timer::ptr &rhs) const
    {
        if (!lhs && !rhs)
        {
            return false;
        }
        if (!lhs)
        {
            return true;
        }
        if (!rhs)
        {
            return false;
        }
        if (lhs->m_next < rhs->m_next)
        {
            return true;
        }
        if (rhs->m_next < lhs->m_next)
        {
            return false;
        }
        return lhs.get() < rhs.get();
    }

    Timer::Timer(uint64_t ms, std::function<void()> cb, bool recurring, TimerManager *manager)
        : m_recurring(recurring), m_ms(ms), m_cb(cb), m_manager(manager)
    {
        m_next = tide::GetCurrentMS() + m_ms;
    }

    Timer::Timer(uint64_t next)
        : m_next(next)
    {
    }

    // 取消定时器，成功返回true，失败返回false
    bool Timer::cancel()
    {
        TimerManager::RWMutexType::WriteLock lock(m_manager->m_mutex);
        // 先置空，再寻找，后删除
        if (m_cb)
        {
            m_cb = nullptr;
            auto it = m_manager->m_timers.find(shared_from_this());

            m_manager->m_timers.erase(it);
            return true;
        }
        return false;
    }

    // 刷新定时器，重新计算下一次执行的时间，并调整在定时器管理器中的位置
    bool Timer::refresh()
    {
        TimerManager::RWMutexType::WriteLock lock(m_manager->m_mutex);
        // 放空判断
        if (!m_cb)
        {
            return false;
        }

        // 寻找位置
        auto it = m_manager->m_timers.find(shared_from_this());
        if (it == m_manager->m_timers.end())
        {
            return false;
        }

        // 删除完修改后重新插入(直接修改会改变位置导致拿到的迭代器失效)
        m_manager->m_timers.erase(it);
        m_next = tide::GetCurrentMS() + m_ms;
        m_manager->m_timers.insert(shared_from_this());

        // 一切正常
        return true;
    }

    // 重置定时器，修改定时器的超时时间，并调整在定时器管理器中的位置
    bool Timer::reset(uint64_t ms, bool from_now)
    {
        if (m_ms == ms && !from_now)
        {
            return true;
        }

        TimerManager::RWMutexType::WriteLock lock(m_manager->m_mutex);
        if (!m_cb)
        {
            return false;
        }

        auto it = m_manager->m_timers.find(shared_from_this());
        if (it == m_manager->m_timers.end())
        {
            return false;
        }

        m_manager->m_timers.erase(it);
        uint64_t start = 0;
        if (from_now)
        {
            start = tide::GetCurrentMS();
        }
        else
        {
            start = m_next - m_ms;
        }
        m_ms = ms;
        m_next = start + m_ms;
        m_manager->addTimer(shared_from_this(), lock);
        return true;
    }

    // TimerManager的实现
    TimerManager::TimerManager()
    {
        m_previouseTime = tide::GetCurrentMS();
    }
    TimerManager::~TimerManager()
    {
    }

    Timer::ptr TimerManager::addTimer(uint64_t ms, std::function<void()> cb, bool recurring)
    {
        Timer::ptr timer(new Timer(ms, cb, recurring, this)); //
        RWMutexType::WriteLock lock(m_mutex);

        addTimer(timer, lock);

        return timer;
    }

    static void OnTimer(std::weak_ptr<void> weak_cond, std::function<void()> cb)
    {
        std::shared_ptr<void> tmp = weak_cond.lock();
        if (tmp)
        {
            cb();
        }
    }

    void TimerManager::addTimer(Timer::ptr val, RWMutexType::WriteLock &lock)
    {
        auto it = m_timers.insert(val).first;
        bool at_front = (it == m_timers.begin()) && !m_tickled;
        if (at_front)
        {
            m_tickled = true;
        }
        lock.unlock();

        if (at_front)
        {
            onTimerInsertedAtFront();
        }
    }

    bool TimerManager::hasTimer()
    {
        RWMutexType::ReadLock lock(m_mutex);
        return !m_timers.empty();
    }

    Timer::ptr TimerManager::addConditionTimer(uint64_t ms, std::function<void()> cb, std::weak_ptr<void> weak_cond, bool recurring)
    {
        return addTimer(ms, std::bind(&OnTimer, weak_cond, cb), recurring);
    }

    uint64_t TimerManager::getNextTimer()
    {
        RWMutexType::ReadLock lock(m_mutex);
        m_tickled = false;
        if (m_timers.empty())
        {
            return ~0ull;
        }

        const Timer::ptr &next = *m_timers.begin();
        uint64_t now_ms = tide::GetCurrentMS();
        if (next->m_next <= now_ms)
        {
            return 0;
        }
        else
        {
            return next->m_next - now_ms;
        }
    }

    // 获取已经超时的定时器回调函数
    void TimerManager::listExpiredCb(std::vector<std::function<void()>> &cbs)
    {
        uint64_t now_ms = tide::GetCurrentMS();
        std::vector<Timer::ptr> expired;
        {
            RWMutexType::ReadLock lock(m_mutex);
            if (m_timers.empty())
            {
                return;
            }
        }
        RWMutexType::WriteLock lock(m_mutex);
        if (m_timers.empty())
        {
            return;
        }
        bool rollover = detectClockRollover(now_ms);
        if (!rollover && ((*m_timers.begin())->m_next > now_ms))
        {
            return;
        }

        Timer::ptr now_timer(new Timer(now_ms));
        auto it = rollover ? m_timers.end() : m_timers.lower_bound(now_timer);
        while (it != m_timers.end() && (*it)->m_next == now_ms)
        {
            ++it;
        }
        expired.insert(expired.begin(), m_timers.begin(), it);
        m_timers.erase(m_timers.begin(), it);
        cbs.reserve(expired.size());

        for (auto &timer : expired)
        {
            cbs.push_back(timer->m_cb);
            if (timer->m_recurring)
            {
                timer->m_next = now_ms + timer->m_ms;
                m_timers.insert(timer);
            }
            else
            {
                timer->m_cb = nullptr;
            }
        }
    }

    // 检测时钟回拨，返回true表示发生了时钟回拨（对当前时间与上次执行时间进行比较后再做时钟回拨）
    bool TimerManager::detectClockRollover(uint64_t now_ms)
    {
        bool rollover = false;
        if (now_ms < m_previouseTime &&
            now_ms < (m_previouseTime - 60 * 60 * 1000))
        {
            rollover = true;
        }
        m_previouseTime = now_ms;
        return rollover;
    }
};