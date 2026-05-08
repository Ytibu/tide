#ifndef TIDE_IOMANAGER_H__
#define TIDE_IOMANAGER_H__

#include "scheduler.h"
#include "thread.h"
#include "macro.h"
#include "timer.h"

namespace tide
{


class IOManager : public Scheduler, public TimerManager
{
public:
    using ptr = std::shared_ptr<IOManager>;
    using RWMutexType = RWMutex;
    
    enum Event{
        NONE = 0x0,
        READ = 0x1,
        WRITE = 0x4
    };

private:
    struct FdContext{
        using MutexType = Mutex;

        struct EventContext{
            Scheduler *scheduler = nullptr; //事件执行的scheduler
            Fiber::ptr fiber;               //事件协程
            std::function<void()> cb;       //事件回调函数
        };

        EventContext& getContext(Event event);  //获取事件上下文
        void resetContext(EventContext& ctx);   //重置事件上下文
        void triggerEvent(Event event);         //触发事件，执行事件回调函数或者协程

        EventContext read;      //读事件
        EventContext write;     //写事件
        int fd = 0;             //事件关联的句柄
        Event m_events = NONE;  //已经注册的事件
        MutexType mutex;
    };


public:
    IOManager(size_t threads = 1, bool use_caller = true, const std::string &name = "");
    ~IOManager();

    int addEvent(int fd, Event event, std::function<void()> cb = nullptr);  //注册事件，成功返回0，失败返回-1
    bool delEvent(int fd, Event event);     //取消事件
    bool cancelEvent(int fd, Event event);  //取消事件，并且触发事件
    bool cancelAll(int fd);         //取消fd的所有事件，并且触发所有事件

    static IOManager* GetThis();

protected:
    void tickle() override;     //通知协程调度器有事件需要被处理
    bool stopping() override;   //是否停止
    bool stopping(uint64_t &timeout);   //是否停止，参数timeout是下一个定时器的执行时间
    void idle() override;       //当没有事件需要处理时，协程调度器的idle协程会执行idle函数，等待事件的发生

    void contextResize(size_t size);    //重置context大小

    void onTimerInsertedAtFront() override;  //当有新的定时器插入，并且插入到第一个位置，执行该函数

private:
    int m_epfd = 0;
    int m_tickleFds[2];

    std::atomic<size_t> m_pendingEventCount = {0};
    RWMutexType m_mutex;
    std::vector<FdContext*> m_fdContexts;
};


};


#endif  // TIDE_IOMANAGER_H__