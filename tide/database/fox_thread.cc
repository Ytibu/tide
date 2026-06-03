#include "fox_thread.h"
#include "../config.h"
#include "../log.h"
#include "../utils.h"
#include "../macro.h"
#include "../config.h"
#include <iomanip>

namespace tide
{

    static tide::Logger::ptr g_logger = TIDE_LOG_NAME("system");

    static tide::ConfigVar<std::map<std::string, std::map<std::string, std::string>>>::ptr g_thread_info_set =
        Config::Lookup("fox_thread", std::map<std::string, std::map<std::string, std::string>>(), "confg for thread");

    static RWMutex s_thread_mutex;

    static std::map<uint64_t, std::string> s_thread_names; // 全局线程信息集合，存储线程ID和名称的映射关系

    thread_local FoxThread *s_thread = nullptr;

    void FoxThread::setThis()
    {
        m_name = m_name + "_" + std::to_string(tide::GetThreadId());
        s_thread = this; // 设置当前线程对象的指针，便于在线程内访问线程相关信息和功能

        RWMutex::WriteLock lock(s_thread_mutex);
        s_thread_names[tide::GetThreadId()] = m_name; // 将当前线程的ID和名称添加到全局线程信息集合中
    }

    void FoxThread::unsetThis()
    {
        s_thread = nullptr; // 取消当前线程对象的指针，适用于线程结束时清理线程相关信息和功能
        RWMutex::WriteLock lock(s_thread_mutex);
        s_thread_names.erase(tide::GetThreadId()); // 从全局线程信息集合中删除当前线程的ID和名称映射关系
    }

    void FoxThread::read_cb(evutil_socket_t sock, short which, void *args)
    {
        FoxThread *thread = static_cast<FoxThread *>(args);
        uint8_t cmd[4096];
        if (::recv(sock, cmd, sizeof(cmd), 0) > 0)
        {
            std::list<callback> callbacks;

            // 将线程的回调函数列表交换到本地变量中，避免在执行回调函数时持有锁，提升性能和并发性
            RWMutex::WriteLock lock(thread->m_mutex);
            callbacks.swap(thread->m_callbacks);
            lock.unlock();

            thread->m_working = true; // 设置线程正在工作的标志

            // 执行所有回调函数，处理线程事件，执行分发的任务
            for (auto it = callbacks.begin(); it != callbacks.end(); ++it)
            {
                if (*it)
                {
                    // TIDE_ASSERT(thread == GetThis());
                    try
                    {
                        (*it)();
                    }
                    catch (std::exception &ex)
                    {
                        TIDE_LOG_ERROR(g_logger) << "exception:" << ex.what();
                    }
                    catch (const char *c)
                    {
                        TIDE_LOG_ERROR(g_logger) << "exception:" << c;
                    }
                    catch (...)
                    {
                        TIDE_LOG_ERROR(g_logger) << "uncatch exception";
                    }
                }
                else
                {
                    event_base_loopbreak(thread->m_base); // 通过调用event_base_loopbreak函数来中断事件循环，停止线程的运行
                    thread->m_start = false;
                    thread->unsetThis(); // 取消当前线程对象的指针，适用于线程结束时清理线程相关信息和功能
                    break;
                }
            }
            tide::Atomic::addFetch(thread->m_total, callbacks.size()); // 原子操作增加线程处理的总任务数
            thread->m_working = false;                                 // 设置线程不再工作的标志
        }
    }

    // 获取当前线程对象的指针
    FoxThread *FoxThread::GetThis()
    {
        return s_thread;
    }

    // 获取当前线程的名称
    const std::string &FoxThread::GetFoxThreadName()
    {
        // 获取当前线程的名称并直接返回
        FoxThread *t = GetThis();
        if (t)
        {
            return t->m_name;
        }

        // 获取当前线程Id，并根据id在全局线程信息集合中查找线程名称，找到后直接返回
        uint64_t tid = tide::GetThreadId();
        do
        {
            RWMutex::ReadLock lock(s_thread_mutex);
            auto it = s_thread_names.find(tid);
            if (it != s_thread_names.end())
            {
                return it->second;
            }
        } while (0);

        // 利用线程ID生成一个默认的线程名称，并将其添加到全局线程信息集合中，最后返回这个默认的线程名称
        do
        {
            RWMutex::WriteLock lock(s_thread_mutex);
            s_thread_names[tid] = "UNNAME_" + std::to_string(tid);
            return s_thread_names[tid];
        } while (0);
    }

    // 将全局线程信息集合直接存储进传入的引用Map中，便于外部访问和使用线程信息
    void FoxThread::GetAllFoxThreadName(std::map<uint64_t, std::string> &names)
    {
        RWMutex::ReadLock lock(s_thread_mutex);
        for (auto it = s_thread_names.begin(); it != s_thread_names.end(); ++it)
        {
            names.insert(*it);
        }
    }

    FoxThread::FoxThread(const std::string &name, struct event_base *base)
        : m_read(0), m_write(0), m_base(NULL), m_event(NULL), m_thread(NULL), m_name(name), m_working(false), m_start(false), m_total(0)
    {
        // 创建一个UNIX域套接字对，用于线程事件的通信和处理，套接字对包含两个文件描述符，一个用于读操作，一个用于写操作
        int fds[2];
        if (evutil_socketpair(AF_UNIX, SOCK_STREAM, 0, fds) == -1)
        {
            // TIDE_LOG_ERROR(g_logger) << "FoxThread init error";
            throw std::logic_error("thread init error");
        }

        // 将线程事件的文件描述符设置为非阻塞模式，便于事件循环中监听线程相关事件时的高效处理
        evutil_make_socket_nonblocking(fds[0]);
        evutil_make_socket_nonblocking(fds[1]);

        // 设置读写端
        m_read = fds[0];
        m_write = fds[1];

        // 如果传入了事件循环基础设施，则使用它并将当前线程对象的指针设置为当前线程；
        if (base)
        {
            m_base = base;
            setThis();
        }
        else // 否则，创建一个新的事件循环并将当前线程对象的指针设置为当前线程
        {
            m_base = event_base_new();
        }

        // 创建一个新的事件，并将其添加到事件循环中，事件的回调函数为read_cb，用于处理线程事件，执行分发的任务
        m_event = event_new(m_base, m_read, EV_READ | EV_PERSIST, read_cb, this);
        event_add(m_event, NULL);
    }

    FoxThread::~FoxThread()
    {
        // 关闭读端和写端文件描述符
        if (m_read)
        {
            close(m_read);
        }
        if (m_write)
        {
            close(m_write);
        }

        // 停止线程的运行，并等待线程结束，最后释放线程相关资源
        stop();
        join();

        // 释放线程相关资源，包括线程对象、事件和事件循环
        if (m_thread)
        {
            delete m_thread;
        }
        if (m_event)
        {
            event_free(m_event);
        }
        if (m_base)
        {
            event_base_free(m_base);
        }
    }

    // 输出线程的状态信息，包括线程名称、是否正在工作、待处理任务数和总任务数
    void FoxThread::dump(std::ostream &os)
    {
        RWMutex::ReadLock lock(m_mutex);
        os << "[thread name=" << m_name
           << " working=" << m_working
           << " tasks=" << m_callbacks.size()
           << " total=" << m_total
           << "]" << std::endl;
    }

    // 获取当前线程的ID，如果线程对象存在则返回线程对象的ID，否则返回一个默认的线程ID
    std::thread::id FoxThread::getId() const
    {
        if (m_thread)
        {
            return m_thread->get_id();
        }
        return std::thread::id();
    }

    // 获取线程数据，根据传入的名称在线程的数据集合中查找对应的数据，如果找到则返回数据指针，否则返回nullptr
    void *FoxThread::getData(const std::string &name)
    {
        // Mutex::ReadLock lock(m_mutex);
        auto it = m_datas.find(name);
        return it == m_datas.end() ? nullptr : it->second;
    }

    void FoxThread::setData(const std::string &name, void *v)
    {
        // Mutex::WriteLock lock(m_mutex);
        m_datas[name] = v;
    }

    void FoxThread::start()
    {
        if (m_thread)
        {
            // TIDE_LOG_ERROR(g_logger) << "FoxThread is running";
            throw std::logic_error("FoxThread is running");
        }

        // 创建一个新的线程，并将线程事件回调函数thread_cb作为线程的入口函数，启动线程的运行
        m_thread = new std::thread(std::bind(&FoxThread::thread_cb, this));
        m_start = true;
    }

    void FoxThread::thread_cb()
    {
        // std::cout << "FoxThread(" << m_name << "," << pthread_self() << ")" << std::endl;
        setThis();

        // 设置线程名称
        pthread_setname_np(pthread_self(), m_name.substr(0, 15).c_str());
        if (m_initCb)
        {
            m_initCb(this);
            m_initCb = nullptr;
        }
        event_base_loop(m_base, 0);
    }

    bool FoxThread::dispatch(callback cb)
    {
        RWMutex::WriteLock lock(m_mutex);
        m_callbacks.push_back(cb);
        // if(m_callbacks.size() > 1) {
        //     std::cout << std::this_thread::get_id() << ":" << m_callbacks.size() << " " << m_name << std::endl;
        // }
        lock.unlock();
        uint8_t cmd = 1;
        // write(m_write, &cmd, sizeof(cmd));
        if (send(m_write, &cmd, sizeof(cmd), 0) <= 0)
        {
            return false;
        }
        return true;
    }

    bool FoxThread::dispatch(uint32_t id, callback cb)
    {
        return dispatch(cb);
    }

    bool FoxThread::batchDispatch(const std::vector<callback> &cbs)
    {
        RWMutex::WriteLock lock(m_mutex);
        for (auto &i : cbs)
        {
            m_callbacks.push_back(i);
        }
        lock.unlock();
        uint8_t cmd = 1;
        if (send(m_write, &cmd, sizeof(cmd), 0) <= 0)
        {
            return false;
        }
        return true;
    }

    void FoxThread::broadcast(callback cb)
    {
        dispatch(cb);
    }

    void FoxThread::stop()
    {
        RWMutex::WriteLock lock(m_mutex);
        m_callbacks.push_back(nullptr);
        if (m_thread)
        {
            uint8_t cmd = 0;
            // write(m_write, &cmd, sizeof(cmd));
            send(m_write, &cmd, sizeof(cmd), 0);
        }
        // if(m_data) {
        //     delete m_data;
        //     m_data = NULL;
        // }
    }

    void FoxThread::join()
    {
        if (m_thread)
        {
            m_thread->join();
            delete m_thread;
            m_thread = NULL;
        }
    }

    FoxThreadPool::FoxThreadPool(uint32_t size, const std::string &name, bool advance)
        : m_size(size), m_cur(0), m_name(name), m_advance(advance), m_start(false), m_total(0)
    {
        m_threads.resize(m_size);
        for (size_t i = 0; i < size; ++i)
        {
            FoxThread *t(new FoxThread(name + "_" + std::to_string(i)));
            m_threads[i] = t;
        }
    }

    FoxThreadPool::~FoxThreadPool()
    {
        for (size_t i = 0; i < m_size; ++i)
        {
            delete m_threads[i];
        }
    }

    void FoxThreadPool::start()
    {
        for (size_t i = 0; i < m_size; ++i)
        {
            m_threads[i]->setInitCb(m_initCb);
            m_threads[i]->start();
            m_freeFoxThreads.push_back(m_threads[i]);
        }
        if (m_initCb)
        {
            m_initCb = nullptr;
        }
        m_start = true;
        check();
    }

    void FoxThreadPool::stop()
    {
        for (size_t i = 0; i < m_size; ++i)
        {
            m_threads[i]->stop();
        }
        m_start = false;
    }

    void FoxThreadPool::join()
    {
        for (size_t i = 0; i < m_size; ++i)
        {
            m_threads[i]->join();
        }
    }

    void FoxThreadPool::releaseFoxThread(FoxThread *t)
    {
        do
        {
            RWMutex::WriteLock lock(m_mutex);
            m_freeFoxThreads.push_back(t);
        } while (0);
        check();
    }

    bool FoxThreadPool::dispatch(callback cb)
    {
        do
        {
            tide::Atomic::addFetch(m_total, (uint64_t)1);
            RWMutex::WriteLock lock(m_mutex);
            if (!m_advance)
            {
                return m_threads[m_cur++ % m_size]->dispatch(cb);
            }
            m_callbacks.push_back(cb);
        } while (0);
        check();
        return true;
    }

    bool FoxThreadPool::batchDispatch(const std::vector<callback> &cbs)
    {
        tide::Atomic::addFetch(m_total, cbs.size());
        RWMutex::WriteLock lock(m_mutex);
        if (!m_advance)
        {
            for (auto cb : cbs)
            {
                m_threads[m_cur++ % m_size]->dispatch(cb);
            }
            return true;
        }
        for (auto cb : cbs)
        {
            m_callbacks.push_back(cb);
        }
        lock.unlock();
        check();
        return true;
    }

    void FoxThreadPool::check()
    {
        do
        {
            if (!m_start)
            {
                break;
            }
            RWMutex::WriteLock lock(m_mutex);
            if (m_freeFoxThreads.empty() || m_callbacks.empty())
            {
                break;
            }

            std::shared_ptr<FoxThread> thr(m_freeFoxThreads.front(),
                                           std::bind(&FoxThreadPool::releaseFoxThread,
                                                     this, std::placeholders::_1));
            m_freeFoxThreads.pop_front();

            callback cb = m_callbacks.front();
            m_callbacks.pop_front();
            lock.unlock();

            if (thr->isStart())
            {
                thr->dispatch(std::bind(&FoxThreadPool::wrapcb, this, thr, cb));
            }
            else
            {
                RWMutex::WriteLock lock(m_mutex);
                m_callbacks.push_front(cb);
            }
        } while (true);
    }

    void FoxThreadPool::wrapcb(std::shared_ptr<FoxThread> thr, callback cb)
    {
        cb();
    }

    bool FoxThreadPool::dispatch(uint32_t id, callback cb)
    {
        tide::Atomic::addFetch(m_total, (uint64_t)1);
        return m_threads[id % m_size]->dispatch(cb);
    }

    FoxThread *FoxThreadPool::getRandFoxThread()
    {
        return m_threads[m_cur++ % m_size];
    }

    void FoxThreadPool::broadcast(callback cb)
    {
        for (size_t i = 0; i < m_threads.size(); ++i)
        {
            m_threads[i]->dispatch(cb);
        }
    }

    void FoxThreadPool::dump(std::ostream &os)
    {
        RWMutex::ReadLock lock(m_mutex);
        os << "[FoxThreadPool name = " << m_name << " thread_count = " << m_threads.size()
           << " tasks = " << m_callbacks.size() << " total = " << m_total
           << " advance = " << m_advance
           << "]" << std::endl;
        for (size_t i = 0; i < m_threads.size(); ++i)
        {
            os << "    ";
            m_threads[i]->dump(os);
        }
    }

    IFoxThread::ptr FoxThreadManager::get(const std::string &name)
    {
        auto it = m_threads.find(name);
        return it == m_threads.end() ? nullptr : it->second;
    }

    void FoxThreadManager::add(const std::string &name, IFoxThread::ptr thr)
    {
        m_threads[name] = thr;
    }

    void FoxThreadManager::dispatch(const std::string &name, callback cb)
    {
        IFoxThread::ptr ti = get(name);
        TIDE_ASSERT(ti);
        ti->dispatch(cb);
    }

    void FoxThreadManager::dispatch(const std::string &name, uint32_t id, callback cb)
    {
        IFoxThread::ptr ti = get(name);
        TIDE_ASSERT(ti);
        ti->dispatch(id, cb);
    }

    void FoxThreadManager::batchDispatch(const std::string &name, const std::vector<callback> &cbs)
    {
        IFoxThread::ptr ti = get(name);
        TIDE_ASSERT(ti);
        ti->batchDispatch(cbs);
    }

    void FoxThreadManager::broadcast(const std::string &name, callback cb)
    {
        IFoxThread::ptr ti = get(name);
        TIDE_ASSERT(ti);
        ti->broadcast(cb);
    }

    void FoxThreadManager::dumpFoxThreadStatus(std::ostream &os)
    {
        os << "FoxThreadManager: " << std::endl;
        for (auto it = m_threads.begin();
             it != m_threads.end(); ++it)
        {
            it->second->dump(os);
        }

        os << "All FoxThreads:" << std::endl;
        std::map<uint64_t, std::string> names;
        FoxThread::GetAllFoxThreadName(names);
        for (auto it = names.begin();
             it != names.end(); ++it)
        {
            os << std::setw(30) << it->first
               << ": " << it->second << std::endl;
        }
    }

    void FoxThreadManager::init()
    {
        auto m = g_thread_info_set->getValue();
        for (auto i : m)
        {
            auto num = tide::GetParamValue(i.second, "num", 0);
            auto name = i.first;
            auto advance = tide::GetParamValue(i.second, "advance", 0);
            if (num <= 0)
            {
                TIDE_LOG_ERROR(g_logger) << "thread pool:" << name
                                         << " num:" << num
                                         << " advance:" << advance
                                         << " invalid";
                continue;
            }
            if (num == 1)
            {
                m_threads[i.first] = FoxThread::ptr(new FoxThread(i.first));
                TIDE_LOG_INFO(g_logger) << "init thread : " << i.first;
            }
            else
            {
                m_threads[i.first] = FoxThreadPool::ptr(new FoxThreadPool(
                    num, name, advance));
                TIDE_LOG_INFO(g_logger) << "init thread pool:" << name
                                        << " num:" << num
                                        << " advance:" << advance;
            }
        }
    }

    void FoxThreadManager::start()
    {
        for (auto i : m_threads)
        {
            TIDE_LOG_INFO(g_logger) << "thread: " << i.first << " start begin";
            i.second->start();
            TIDE_LOG_INFO(g_logger) << "thread: " << i.first << " start end";
        }
    }

    void FoxThreadManager::stop()
    {
        for (auto i : m_threads)
        {
            TIDE_LOG_INFO(g_logger) << "thread: " << i.first << " stop begin";
            i.second->stop();
            TIDE_LOG_INFO(g_logger) << "thread: " << i.first << " stop end";
        }
        for (auto i : m_threads)
        {
            TIDE_LOG_INFO(g_logger) << "thread: " << i.first << " join begin";
            i.second->join();
            TIDE_LOG_INFO(g_logger) << "thread: " << i.first << " join end";
        }
    }

}