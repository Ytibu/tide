#ifndef TIDE_DB_FOX_THREAD_H__
#define TIDE_DB_FOX_THREAD_H__

#include <thread>
#include <vector>
#include <list>
#include <map>
#include <string>

#include <event2/bufferevent.h>
#include <event2/buffer.h>
#include <event2/listener.h>

#include "../singleton.h"
#include "../mutex.h"

namespace tide
{

    class FoxThread;
    class IFoxThread    // 线程接口类，定义了线程的基本功能和行为
    {
    public:
        typedef std::shared_ptr<IFoxThread> ptr;
        typedef std::function<void()> callback;

        virtual ~IFoxThread() {};

        /**
         * @brief 分发一个任务到线程中，线程会执行该任务
         *
         * @param cb
         * @return true
         * @return false
         */
        virtual bool dispatch(callback cb) = 0;

        /**
         * @brief 根据线程ID分发一个任务到线程中，线程会执行该任务
         *
         * @param id
         * @param cb
         * @return true
         * @return false
         */
        virtual bool dispatch(uint32_t id, callback cb) = 0;

        /**
         * @brief 批量分发任务到线程中，适用于需要同时执行多个任务的场景
         *
         * @param cbs
         * @return true
         * @return false
         */
        virtual bool batchDispatch(const std::vector<callback> &cbs) = 0;

        /**
         * @brief 向线程中广播一个任务，适用于需要在所有线程上执行相同任务的场景
         *
         * @param cb
         */
        virtual void broadcast(callback cb) = 0;

        virtual void start() = 0;
        virtual void stop() = 0;
        virtual void join() = 0;

        /**
         * @brief 获取线程池中所有线程的信息
         *
         * @param os
         */
        virtual void dump(std::ostream &os) = 0;

        /**
         * @brief 获取线程处理的总任务数，便于监控和调试
         *
         * @return uint64_t
         */
        virtual uint64_t getTotal() = 0;
    };

    // 线程类，继承自线程接口类，实现了线程的具体功能和行为
    class FoxThread : public IFoxThread
    {
    public:
        typedef std::shared_ptr<FoxThread> ptr;
        typedef IFoxThread::callback callback;
        typedef std::function<void(FoxThread *)> init_cb;

        /**
         * @brief 构造一个线程对象
         *
         * @param name 线程名称，便于调试和监控
         * @param base 用于指定线程使用的事件循环基础设施，如果为 `NULL`，则线程会创建自己的事件循环
         */
        FoxThread(const std::string &name = "", struct event_base *base = NULL);
        ~FoxThread();

        /**
         * @brief 获取当前线程对象的指针，便于在线程内访问线程相关信息和功能
         *
         * @return FoxThread*
         */
        static FoxThread *GetThis();

        /**
         * @brief 获取当前线程的名称，便于调试和监控
         *
         * @return const std::string&
         */
        static const std::string &GetFoxThreadName();

        /**
         * @brief 获取所有线程的ID和名称，便于调试和监控
         *
         * @param names
         */
        static void GetAllFoxThreadName(std::map<uint64_t, std::string> &names);

        /**
         * @brief 将线程对象的指针设置为当前线程，适用于线程开始时初始化线程相关信息和功能
         *
         */
        void setThis();

        /**
         * @brief 取消当前线程对象的指针，适用于线程结束时清理线程相关信息和功能
         *
         */
        void unsetThis();

        void start();

        // 一系列的任务添加
        virtual bool dispatch(callback cb);
        virtual bool dispatch(uint32_t id, callback cb);
        virtual bool batchDispatch(const std::vector<callback> &cbs);
        virtual void broadcast(callback cb);

        void join();
        void stop();
        bool isStart() const { return m_start; }

        struct event_base *getBase() { return m_base; }
        std::thread::id getId() const;

        void *getData(const std::string &name);
        template <class T>
        T *getData(const std::string &name)
        {
            return (T *)getData(name);
        }
        void setData(const std::string &name, void *v);

        void setInitCb(init_cb v) { m_initCb = v; }

        void dump(std::ostream &os);
        virtual uint64_t getTotal() { return m_total; }

    private:
        /**
         * @brief 线程事件回调函数，用于处理线程事件，执行分发的任务
         * 
         */
        void thread_cb();

        /**
         * @brief 线程事件回调函数，用于处理线程事件，执行分发的任务
         *
         * @param sock 线程事件的文件描述符，通常用于事件循环中监听线程相关事件
         * @param which 线程事件的类型，通常用于区分不同类型的事件，如读事件、写事件等
         * @param args 线程事件的用户数据，我们通过直接传递线程对象指针来访问线程相关信息和功能
         */
        static void read_cb(evutil_socket_t sock, short which, void *args);

    private:
        evutil_socket_t m_read;
        evutil_socket_t m_write;
        struct event_base *m_base;
        struct event *m_event;
        std::thread *m_thread;
        tide::RWMutex m_mutex;
        std::list<callback> m_callbacks;

        std::string m_name;
        init_cb m_initCb;

        std::map<std::string, void *> m_datas;

        bool m_working;
        bool m_start;
        uint64_t m_total;
    };

    class FoxThreadPool : public IFoxThread
    {
    public:
        typedef std::shared_ptr<FoxThreadPool> ptr;
        typedef IFoxThread::callback callback;

        /**
         * @brief 构造一个线程池，
         * 线程池中的线程数量由 `size` 参数指定，线程池名称由 `name` 参数指定，`advance` 参数用于控制线程池的调度策略
         *
         * @param size
         * @param name
         * @param advance
         */
        FoxThreadPool(uint32_t size, const std::string &name = "", bool advance = false);
        ~FoxThreadPool();

        void start();
        void stop();
        void join();

        /**
         * @brief 分发一个任务到线程池中的线程，线程池会根据调度策略选择一个线程来执行任务，
         * 如果 `advance` 为 `false`，则采用轮询方式分配任务；
         * 如果 `advance` 为 `true`，则将任务放入队列中，由空闲的线程来获取并执行任务
         *
         * @param cb
         * @return true
         * @return false
         */
        bool dispatch(callback cb);

        /**
         * @brief 批量分发任务到线程池中的线程，适用于需要同时在多个线程上执行相似任务的场景
         *
         * @param cb
         * @return true
         * @return false
         */
        bool batchDispatch(const std::vector<callback> &cb);

        /**
         * @brief 根据线程ID分发一个任务到线程池中的线程，线程池会根据ID选择对应线程来执行任务，
         *
         * @param id
         * @param cb
         * @return true
         * @return false
         */
        bool dispatch(uint32_t id, callback cb);

        /**
         * @brief 获取一个随机的线程池中的线程，适用于需要在任意线程上执行任务的场景
         *
         * @return FoxThread*
         */
        FoxThread *getRandFoxThread();

        /**
         * @brief 向线程池中的所有线程广播一个任务，适用于需要在所有线程上执行相同任务的场景
         *
         * @param v
         */
        void setInitCb(FoxThread::init_cb v) { m_initCb = v; }

        void dump(std::ostream &os);

        void broadcast(callback cb);
        virtual uint64_t getTotal() { return m_total; }

    private:
        void releaseFoxThread(FoxThread *t);
        void check();

        void wrapcb(std::shared_ptr<FoxThread>, callback cb);

    private:
        uint32_t m_size;
        uint32_t m_cur;
        std::string m_name;
        bool m_advance;
        bool m_start;
        RWMutex m_mutex;
        std::list<callback> m_callbacks;
        std::vector<FoxThread *> m_threads;
        std::list<FoxThread *> m_freeFoxThreads;
        FoxThread::init_cb m_initCb;
        uint64_t m_total;
    };

    class FoxThreadManager
    {
    public:
        typedef IFoxThread::callback callback;

        /**
         * @brief 根据线程名称分发任务
         *
         * @param name
         * @param cb
         */
        void dispatch(const std::string &name, callback cb);

        /**
         * @brief 根据线程名称和ID分发任务，ID用于选择线程池中的具体线程
         *
         * @param name
         * @param id
         * @param cb
         */
        void dispatch(const std::string &name, uint32_t id, callback cb);

        /**
         * @brief 根据线程名称批量分发任务，适用于需要同时在多个线程上执行相似任务的场景
         *
         * @param name
         * @param cbs
         */
        void batchDispatch(const std::string &name, const std::vector<callback> &cbs);

        /**
         * @brief 向线程池中的所有线程广播任务，适用于需要在所有线程上执行相同任务的场景
         *
         * @param name
         * @param cb
         */
        void broadcast(const std::string &name, callback cb);

        /**
         * @brief 输出所有线程的状态信息，便于监控和调试
         *
         * @param os
         */
        void dumpFoxThreadStatus(std::ostream &os);

        void init();
        void start();
        void stop();

        IFoxThread::ptr get(const std::string &name);

        /**
         * @brief 添加一个线程到管理器中，线程通过名称进行标识，便于后续的任务分发
         *
         * @param name
         * @param thr
         */
        void add(const std::string &name, IFoxThread::ptr thr);

    private:
        std::map<std::string, IFoxThread::ptr> m_threads; // 线程名称到线程对象的映射
    };

    typedef Singleton<FoxThreadManager> FoxThreadMgr; // 全局线程管理器单例

}
#endif